#include "version_detect.h"
#include <stdio.h>
#include <string.h>

// ��ȡWindows�汾��Ϣ���ִ�����
static BOOL GetWindowsVersionInfo(DWORD* major, DWORD* minor, DWORD* build) {
    // ʹ��RtlGetVersion��ȡ��ʵ�汾��Ϣ���ƹ������Բ㣩
    typedef NTSTATUS (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion) {
            RTL_OSVERSIONINFOW osInfo = { 0 };
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            
            if (RtlGetVersion(&osInfo) == 0) {
                *major = osInfo.dwMajorVersion;
                *minor = osInfo.dwMinorVersion;
                *build = osInfo.dwBuildNumber;
                return TRUE;
            }
        }
    }
    
    // ���÷�����ʹ��GetVersionEx
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        *major = osvi.dwMajorVersion;
        *minor = osvi.dwMinorVersion;
        *build = osvi.dwBuildNumber;
        return TRUE;
    }
    
    return FALSE;
}

WindowsVersion detect_windows_version() {
    DWORD major, minor, build;
    
    if (!GetWindowsVersionInfo(&major, &minor, &build)) {
        return WIN_UNKNOWN;
    }
    
    // Windows�汾�ж��߼�
    if (major == 6) {
        if (minor == 2) return WIN_8;      // Windows 8
        if (minor == 3) return WIN_8_1;    // Windows 8.1
    } else if (major == 10) {
        if (build >= 22000) return WIN_11; // Windows 11 (Build 22000+)
        return WIN_10;                     // Windows 10
    }
    
    return WIN_UNKNOWN;
}

BOOL is_windows_8_or_later() {
    WindowsVersion version = detect_windows_version();
    return version >= WIN_8;
}

BOOL is_windows_10_or_later() {
    WindowsVersion version = detect_windows_version();
    return version >= WIN_10;
}

BOOL is_windows_11_or_later() {
    WindowsVersion version = detect_windows_version();
    return version >= WIN_11;
}

const char* get_windows_version_string() {
    WindowsVersion version = detect_windows_version();
    
    switch (version) {
        case WIN_8:     return "Windows 8";
        case WIN_8_1:   return "Windows 8.1";
        case WIN_10:    return "Windows 10";
        case WIN_11:    return "Windows 11";
        default:        return "Unknown Windows Version";
    }
}

BOOL check_windows_compatibility() {
    WindowsVersion version = detect_windows_version();
    
    // ����Ƿ�Ϊ֧�ֵ�Windows�汾
    if (version < WIN_8) {
        printf("����: ��֧�ֵ�Windows�汾����ҪWindows 8����߰汾\n");
        return FALSE;
    }
    
    printf("��⵽Windows�汾: %s\n", get_windows_version_string());
    
    // ���ݰ汾�ṩ�ض�����
    switch (version) {
        case WIN_8:
        case WIN_8_1:
            printf("��ʾ: Windows 8/8.1�����������Թ���ԱȨ������\n");
            break;
            
        case WIN_10:
            printf("��ʾ: Windows 10������ע��Windows Defender��������\n");
            break;
            
        case WIN_11:
            printf("��ʾ: Windows 11��������ȫ���Ը��ϸ񣬿�����Ҫ��������\n");
            break;
            
        default:
            break;
    }
    
    return TRUE;
}
