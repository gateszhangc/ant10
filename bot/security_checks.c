#include "security_checks.h"
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <psapi.h>
#include <winternl.h> // 提供 PEB 结构定义

// 反调试检测函数实现
BOOL anti_debug() {
    // 技术1: 使用 PEB 直接检测调试器
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60); // x64 架构
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30); // x86 架构
#endif
    
    if (pPeb->BeingDebugged) {
        return TRUE;
    }

    // 技术2: 检查父进程名
    DWORD parentPid = 0;
    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ProcessID == GetCurrentProcessId()) {
                    parentPid = pe32.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    
    if (parentPid != 0) {
        HANDLE hParent = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, parentPid);
        if (hParent) {
            CHAR parentName[MAX_PATH] = {0};
            if (GetModuleFileNameExA(hParent, NULL, parentName, sizeof(parentName))) {
                // 匹配常见调试器进程名
                if (strstr(parentName, "ollydbg") || 
                    strstr(parentName, "ida") ||
                    strstr(parentName, "x32dbg") || 
                    strstr(parentName, "x64dbg") ||
                    strstr(parentName, "windbg") ||
                    strstr(parentName, "devenv")) {
                    CloseHandle(hParent);
                    return TRUE;
                }
            }
            CloseHandle(hParent);
        }
    }

    // 技术3: NtQueryInformationProcess 方法检测调试器
    typedef NTSTATUS (NTAPI *pNtQueryInformationProcess)(
        HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        pNtQueryInformationProcess NtQueryInformationProcess = 
            (pNtQueryInformationProcess)GetProcAddress(ntdll, "NtQueryInformationProcess");
        
        if (NtQueryInformationProcess) {
            ULONG debugPort = 0;
            NTSTATUS status = NtQueryInformationProcess(
                GetCurrentProcess(),
                ProcessDebugPort,
                &debugPort,
                sizeof(debugPort),
                NULL
            );
            
            if (NT_SUCCESS(status) && debugPort != 0) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

// 用户行为检测函数实现
BOOL check_user_activity() {
    // // 使用 C 标准库的 rand() 实现随机延迟
    // int delay = 5000 + (rand() % 3000); // 1000-3000ms 随机延迟
    // Sleep(delay);

    // // 检测鼠标移动情况
    // POINT pt1, pt2;
    // GetCursorPos(&pt1);
    // Sleep(delay); // 使用相同的随机延迟
    // GetCursorPos(&pt2);
    
    // // 计算鼠标移动距离
    // int deltaX = abs(pt1.x - pt2.x);
    // int deltaY = abs(pt1.y - pt2.y);
    
    // 如果移动距离小于5像素视为未移动
    // if (deltaX < 5 && deltaY < 5) {
    //     return TRUE;
    // }

    // 检测磁盘空间
    ULARGE_INTEGER freeBytes;
    if (GetDiskFreeSpaceExA("C:\\", NULL, NULL, &freeBytes)) {
        // 20GB 阈值
        const ULONGLONG minDiskSpace = 20 * 1024 * 1024 * 1024ULL;
        if (freeBytes.QuadPart < minDiskSpace) {
            return TRUE;
        }
    }

    // // 检测系统运行时间
    // ULONGLONG uptime = GetTickCount64();
    // // 1小时 = 3600 * 1000 = 3,600,000 毫秒
    // if (uptime < 10 * 60 * 1000 ) {
    //     return TRUE;
    // }

    return FALSE;
}