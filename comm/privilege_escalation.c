#include "privilege_escalation.h"
#include "version_detect.h"
#include <stdio.h>
#include <shellapi.h>

BOOL is_running_as_admin() {
    BOOL is_admin = FALSE;
    PSID admin_group = NULL;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    
    // 创建管理员组SID
    if (AllocateAndInitializeSid(&nt_authority, 2, 
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &admin_group)) {
        
        // 检查当前用户是否在管理员组中
        if (!CheckTokenMembership(NULL, admin_group, &is_admin)) {
            is_admin = FALSE;
        }
        
        FreeSid(admin_group);
    }
    
    return is_admin;
}

BOOL request_admin_privileges() {
    if (is_running_as_admin()) {
        printf("已经具有管理员权限\n");
        return TRUE;
    }
    
    printf("请求管理员权限...\n");
    
    // 获取当前程序路径
    WCHAR exe_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, exe_path, MAX_PATH) == 0) {
        printf("无法获取程序路径\n");
        return FALSE;
    }
    
    // 使用ShellExecute请求管理员权限
    SHELLEXECUTEINFOW sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";  // 请求管理员权限
    sei.lpFile = exe_path;
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;
    
    if (!ShellExecuteExW(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            printf("用户取消了权限提升请求\n");
        } else {
            printf("权限提升失败，错误代码: %lu\n", error);
        }
        return FALSE;
    }
    
    // 如果成功启动了新进程，当前进程应该退出
    printf("正在以管理员权限重新启动...\n");
    ExitProcess(0);
    
    return TRUE;
}

BOOL enable_privilege(LPCWSTR privilege_name) {
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    // 打开当前进程的访问令牌
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        printf("无法打开进程令牌\n");
        return FALSE;
    }
    
    // 查找权限的LUID
    if (!LookupPrivilegeValueW(NULL, privilege_name, &luid)) {
        printf("无法查找权限值\n");
        CloseHandle(token);
        return FALSE;
    }
    
    // 设置权限
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    // 调整权限
    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        printf("无法调整权限\n");
        CloseHandle(token);
        return FALSE;
    }
    
    // 检查是否成功
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        printf("权限未完全分配\n");
        CloseHandle(token);
        return FALSE;
    }
    
    CloseHandle(token);
    return TRUE;
}

BOOL enable_debug_privilege() {
    printf("启用调试权限...\n");
    return enable_privilege(L"SeDebugPrivilege");
}

BOOL is_uac_enabled() {
    HKEY key;
    DWORD value = 0;
    DWORD size = sizeof(value);
    
    // 检查UAC注册表设置
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                     0, KEY_READ, &key) == ERROR_SUCCESS) {
        
        RegQueryValueExW(key, L"EnableLUA", NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(key);
    }
    
    return (value != 0);
}

BOOL bypass_uac_if_possible() {
    WindowsVersion version = detect_windows_version();
    
    printf("检查UAC绕过可能性...\n");
    
    // 在Windows 8及以后版本中，UAC绕过更加困难
    if (version >= WIN_8) {
        printf("Windows 8+环境，UAC绕过受限\n");
        
        // 检查是否已经是管理员
        if (is_running_as_admin()) {
            printf("已具有管理员权限，无需绕过UAC\n");
            return TRUE;
        }
        
        // 尝试请求权限提升
        return request_admin_privileges();
    }
    
    printf("较旧的Windows版本，可能存在UAC绕过方法\n");
    return FALSE;
}

BOOL adjust_process_privileges() {
    BOOL success = TRUE;
    
    printf("调整进程权限...\n");
    
    // 启用调试权限
    if (!enable_debug_privilege()) {
        printf("警告: 无法启用调试权限\n");
        success = FALSE;
    }
    
    // 根据Windows版本调整其他权限
    WindowsVersion version = detect_windows_version();
    
    if (version >= WIN_10) {
        // Windows 10/11需要更多权限
        if (!enable_privilege(L"SeIncreaseQuotaPrivilege")) {
            printf("警告: 无法启用配额权限\n");
        }
        
        if (!enable_privilege(L"SeAssignPrimaryTokenPrivilege")) {
            printf("警告: 无法启用主令牌权限\n");
        }
    }
    
    return success;
}

BOOL check_required_privileges() {
    printf("检查所需权限...\n");
    
    // 检查是否为管理员
    if (!is_running_as_admin()) {
        printf("警告: 未以管理员权限运行\n");
        
        WindowsVersion version = detect_windows_version();
        if (version >= WIN_10) {
            printf("Windows 10/11环境强烈建议管理员权限\n");
            return FALSE;
        }
    }
    
    // 检查UAC状态
    if (is_uac_enabled()) {
        printf("UAC已启用，某些操作可能受限\n");
    }
    
    // 尝试调整权限
    return adjust_process_privileges();
}
