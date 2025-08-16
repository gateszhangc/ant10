#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// Windows 8-11���ݵ�stubloader

// Windows�汾���
typedef enum {
    WIN_UNKNOWN = 0,
    WIN_8 = 1,
    WIN_8_1 = 2,
    WIN_10 = 3,
    WIN_11 = 4
} WindowsVersion;

// �򻯵İ汾���
WindowsVersion detect_windows_version() {
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        if (osvi.dwMajorVersion == 6) {
            if (osvi.dwMinorVersion == 2) return WIN_8;
            if (osvi.dwMinorVersion == 3) return WIN_8_1;
        } else if (osvi.dwMajorVersion == 10) {
            if (osvi.dwBuildNumber >= 22000) return WIN_11;
            return WIN_10;
        }
    }
    
    return WIN_UNKNOWN;
}

const char* get_version_string(WindowsVersion version) {
    switch (version) {
        case WIN_8:     return "Windows 8";
        case WIN_8_1:   return "Windows 8.1";
        case WIN_10:    return "Windows 10";
        case WIN_11:    return "Windows 11";
        default:        return "Unknown";
    }
}

// ������ԱȨ��
BOOL is_admin() {
    BOOL is_admin = FALSE;
    PSID admin_group = NULL;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&nt_authority, 2, 
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &admin_group)) {
        CheckTokenMembership(NULL, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    
    return is_admin;
}

// ������
int main() {
    printf("=== Windows 8-11����Stubloader ===\n");
    
    WindowsVersion version = detect_windows_version();
    printf("��⵽ϵͳ: %s\n", get_version_string(version));
    
    if (version == WIN_UNKNOWN) {
        printf("����: δ֪��Windows�汾\n");
    }
    
    printf("����ԱȨ��: %s\n", is_admin() ? "��" : "��");
    
    // ���ݰ汾������Ϊ
    switch (version) {
        case WIN_8:
        case WIN_8_1:
            printf("Windows 8/8.1ģʽ: ʹ�ô�ͳAPI\n");
            break;
        case WIN_10:
            printf("Windows 10ģʽ: ע�ⰲȫ����\n");
            break;
        case WIN_11:
            printf("Windows 11ģʽ: �ϸ�ȫ����\n");
            break;
        default:
            printf("����ģʽ: ʹ�û�������\n");
            break;
    }
    
    // ������ʵ�ʵ�payloadִ���߼�
    printf("Stubloaderִ�����\n");
    
    return 0;
}
