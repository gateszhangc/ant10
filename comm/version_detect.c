#include "version_detect.h"
#include <stdio.h>
#include <string.h>

// 获取Windows版本信息的现代方法
static BOOL GetWindowsVersionInfo(DWORD* major, DWORD* minor, DWORD* build) {
    // 使用RtlGetVersion获取真实版本信息（绕过兼容性层）
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
    
    // 备用方法：使用GetVersionEx
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
    
    // Windows版本判断逻辑
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
    
    // 检查是否为支持的Windows版本
    if (version < WIN_8) {
        printf("警告: 不支持的Windows版本，需要Windows 8或更高版本\n");
        return FALSE;
    }
    
    printf("检测到Windows版本: %s\n", get_windows_version_string());
    
    // 根据版本提供特定建议
    switch (version) {
        case WIN_8:
        case WIN_8_1:
            printf("提示: Windows 8/8.1环境，建议以管理员权限运行\n");
            break;
            
        case WIN_10:
            printf("提示: Windows 10环境，注意Windows Defender可能拦截\n");
            break;
            
        case WIN_11:
            printf("提示: Windows 11环境，安全策略更严格，可能需要额外配置\n");
            break;
            
        default:
            break;
    }
    
    return TRUE;
}
