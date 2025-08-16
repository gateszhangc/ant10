#include "privilege_escalation.h"
#include "version_detect.h"
#include <stdio.h>
#include <shellapi.h>

BOOL is_running_as_admin() {
    BOOL is_admin = FALSE;
    PSID admin_group = NULL;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    
    // ��������Ա��SID
    if (AllocateAndInitializeSid(&nt_authority, 2, 
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &admin_group)) {
        
        // ��鵱ǰ�û��Ƿ��ڹ���Ա����
        if (!CheckTokenMembership(NULL, admin_group, &is_admin)) {
            is_admin = FALSE;
        }
        
        FreeSid(admin_group);
    }
    
    return is_admin;
}

BOOL request_admin_privileges() {
    if (is_running_as_admin()) {
        printf("�Ѿ����й���ԱȨ��\n");
        return TRUE;
    }
    
    printf("�������ԱȨ��...\n");
    
    // ��ȡ��ǰ����·��
    WCHAR exe_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, exe_path, MAX_PATH) == 0) {
        printf("�޷���ȡ����·��\n");
        return FALSE;
    }
    
    // ʹ��ShellExecute�������ԱȨ��
    SHELLEXECUTEINFOW sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";  // �������ԱȨ��
    sei.lpFile = exe_path;
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;
    
    if (!ShellExecuteExW(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            printf("�û�ȡ����Ȩ����������\n");
        } else {
            printf("Ȩ������ʧ�ܣ��������: %lu\n", error);
        }
        return FALSE;
    }
    
    // ����ɹ��������½��̣���ǰ����Ӧ���˳�
    printf("�����Թ���ԱȨ����������...\n");
    ExitProcess(0);
    
    return TRUE;
}

BOOL enable_privilege(LPCWSTR privilege_name) {
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    // �򿪵�ǰ���̵ķ�������
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        printf("�޷��򿪽�������\n");
        return FALSE;
    }
    
    // ����Ȩ�޵�LUID
    if (!LookupPrivilegeValueW(NULL, privilege_name, &luid)) {
        printf("�޷�����Ȩ��ֵ\n");
        CloseHandle(token);
        return FALSE;
    }
    
    // ����Ȩ��
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    // ����Ȩ��
    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        printf("�޷�����Ȩ��\n");
        CloseHandle(token);
        return FALSE;
    }
    
    // ����Ƿ�ɹ�
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        printf("Ȩ��δ��ȫ����\n");
        CloseHandle(token);
        return FALSE;
    }
    
    CloseHandle(token);
    return TRUE;
}

BOOL enable_debug_privilege() {
    printf("���õ���Ȩ��...\n");
    return enable_privilege(L"SeDebugPrivilege");
}

BOOL is_uac_enabled() {
    HKEY key;
    DWORD value = 0;
    DWORD size = sizeof(value);
    
    // ���UACע�������
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
    
    printf("���UAC�ƹ�������...\n");
    
    // ��Windows 8���Ժ�汾�У�UAC�ƹ���������
    if (version >= WIN_8) {
        printf("Windows 8+������UAC�ƹ�����\n");
        
        // ����Ƿ��Ѿ��ǹ���Ա
        if (is_running_as_admin()) {
            printf("�Ѿ��й���ԱȨ�ޣ������ƹ�UAC\n");
            return TRUE;
        }
        
        // ��������Ȩ������
        return request_admin_privileges();
    }
    
    printf("�Ͼɵ�Windows�汾�����ܴ���UAC�ƹ�����\n");
    return FALSE;
}

BOOL adjust_process_privileges() {
    BOOL success = TRUE;
    
    printf("��������Ȩ��...\n");
    
    // ���õ���Ȩ��
    if (!enable_debug_privilege()) {
        printf("����: �޷����õ���Ȩ��\n");
        success = FALSE;
    }
    
    // ����Windows�汾��������Ȩ��
    WindowsVersion version = detect_windows_version();
    
    if (version >= WIN_10) {
        // Windows 10/11��Ҫ����Ȩ��
        if (!enable_privilege(L"SeIncreaseQuotaPrivilege")) {
            printf("����: �޷��������Ȩ��\n");
        }
        
        if (!enable_privilege(L"SeAssignPrimaryTokenPrivilege")) {
            printf("����: �޷�����������Ȩ��\n");
        }
    }
    
    return success;
}

BOOL check_required_privileges() {
    printf("�������Ȩ��...\n");
    
    // ����Ƿ�Ϊ����Ա
    if (!is_running_as_admin()) {
        printf("����: δ�Թ���ԱȨ������\n");
        
        WindowsVersion version = detect_windows_version();
        if (version >= WIN_10) {
            printf("Windows 10/11����ǿ�ҽ������ԱȨ��\n");
            return FALSE;
        }
    }
    
    // ���UAC״̬
    if (is_uac_enabled()) {
        printf("UAC�����ã�ĳЩ������������\n");
    }
    
    // ���Ե���Ȩ��
    return adjust_process_privileges();
}
