#ifndef VERSION_DETECT_H
#define VERSION_DETECT_H

#include <windows.h>

// Windows�汾���
typedef enum {
    WIN_UNKNOWN = 0,
    WIN_8 = 1,
    WIN_8_1 = 2,
    WIN_10 = 3,
    WIN_11 = 4
} WindowsVersion;

// �汾��⺯��
WindowsVersion detect_windows_version();
BOOL is_windows_8_or_later();
BOOL is_windows_10_or_later();
BOOL is_windows_11_or_later();

// �����Լ�麯��
BOOL check_windows_compatibility();
const char* get_windows_version_string();

#endif // VERSION_DETECT_H
