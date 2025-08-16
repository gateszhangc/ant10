#include "security_checks.h"
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <psapi.h>
#include <winternl.h> // �ṩ PEB �ṹ����

// �����Լ�⺯��ʵ��
BOOL anti_debug() {
    // ����1: ʹ�� PEB ֱ�Ӽ�������
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60); // x64 �ܹ�
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30); // x86 �ܹ�
#endif
    
    if (pPeb->BeingDebugged) {
        return TRUE;
    }

    // ����2: ��鸸������
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
                // ƥ�䳣��������������
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

    // ����3: NtQueryInformationProcess ������������
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

// �û���Ϊ��⺯��ʵ��
BOOL check_user_activity() {
    // // ʹ�� C ��׼��� rand() ʵ������ӳ�
    // int delay = 5000 + (rand() % 3000); // 1000-3000ms ����ӳ�
    // Sleep(delay);

    // // �������ƶ����
    // POINT pt1, pt2;
    // GetCursorPos(&pt1);
    // Sleep(delay); // ʹ����ͬ������ӳ�
    // GetCursorPos(&pt2);
    
    // // ��������ƶ�����
    // int deltaX = abs(pt1.x - pt2.x);
    // int deltaY = abs(pt1.y - pt2.y);
    
    // ����ƶ�����С��5������Ϊδ�ƶ�
    // if (deltaX < 5 && deltaY < 5) {
    //     return TRUE;
    // }

    // �����̿ռ�
    ULARGE_INTEGER freeBytes;
    if (GetDiskFreeSpaceExA("C:\\", NULL, NULL, &freeBytes)) {
        // 20GB ��ֵ
        const ULONGLONG minDiskSpace = 20 * 1024 * 1024 * 1024ULL;
        if (freeBytes.QuadPart < minDiskSpace) {
            return TRUE;
        }
    }

    // // ���ϵͳ����ʱ��
    // ULONGLONG uptime = GetTickCount64();
    // // 1Сʱ = 3600 * 1000 = 3,600,000 ����
    // if (uptime < 10 * 60 * 1000 ) {
    //     return TRUE;
    // }

    return FALSE;
}