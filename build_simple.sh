#!/bin/bash

# �򻯵ı���ű� - �޸���������

echo "=== ����Windows�����Թ��� (�޸���) ==="
echo

# ��������
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "��װMinGW-w64������..."
    sudo apt update
    sudo apt install -y gcc-mingw-w64-x86-64 gcc-mingw-w64-i686
fi

# ���ñ�����
GCC64="x86_64-w64-mingw32-gcc"
GCC32="i686-w64-mingw32-gcc"

# ����ѡ��
CFLAGS="-Wall -O2 -s"
LIBS="-lkernel32 -luser32 -ladvapi32 -lshell32"

# �������Ŀ¼
mkdir -p enhanced_builds
rm -f enhanced_builds/*.exe enhanced_builds/*.o

echo "1. ������ǿ��stubloader (���ļ��汾)..."

# �������ļ��汾��stubloader
cat << 'EOF' > prop/stubloader_win8to11.c
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
EOF

# ����64λ�汾
echo "����64λ�汾..."
$GCC64 $CFLAGS prop/stubloader_win8to11.c $LIBS -o enhanced_builds/stubloader_win8to11_x64.exe

# ����32λ�汾
echo "����32λ�汾..."
$GCC32 $CFLAGS prop/stubloader_win8to11.c $LIBS -o enhanced_builds/stubloader_win8to11_x86.exe

echo "2. ������ǿ��binder..."

# �����򻯵�binder
cat << 'EOF' > prop/binder_win8to11.c
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== Windows 8-11����Binder ===\n");
    
    if (argc < 3) {
        printf("�÷�: %s <stubloader.exe> <payload.exe> [����ļ�]\n", argv[0]);
        return 1;
    }
    
    char* stub_path = argv[1];
    char* payload_path = argv[2];
    char* output_path = (argc > 3) ? argv[3] : "output_win8to11.exe";
    
    printf("Stubloader: %s\n", stub_path);
    printf("Payload: %s\n", payload_path);
    printf("���: %s\n", output_path);
    
    // ��ȡstubloader
    FILE* stub_file = fopen(stub_path, "rb");
    if (!stub_file) {
        printf("����: �޷���stubloader�ļ�\n");
        return 1;
    }
    
    fseek(stub_file, 0, SEEK_END);
    long stub_size = ftell(stub_file);
    fseek(stub_file, 0, SEEK_SET);
    
    char* stub_data = malloc(stub_size);
    fread(stub_data, 1, stub_size, stub_file);
    fclose(stub_file);
    
    // ��ȡpayload
    FILE* payload_file = fopen(payload_path, "rb");
    if (!payload_file) {
        printf("����: �޷���payload�ļ�\n");
        free(stub_data);
        return 1;
    }
    
    fseek(payload_file, 0, SEEK_END);
    long payload_size = ftell(payload_file);
    fseek(payload_file, 0, SEEK_SET);
    
    char* payload_data = malloc(payload_size);
    fread(payload_data, 1, payload_size, payload_file);
    fclose(payload_file);
    
    // ��������ļ�
    FILE* output_file = fopen(output_path, "wb");
    if (!output_file) {
        printf("����: �޷���������ļ�\n");
        free(stub_data);
        free(payload_data);
        return 1;
    }
    
    // д��stubloader
    fwrite(stub_data, 1, stub_size, output_file);
    
    // д����
    const char marker[] = "WIN8TO11_PAYLOAD_MARKER";
    fwrite(marker, 1, sizeof(marker), output_file);
    
    // д��payload��С
    fwrite(&payload_size, sizeof(payload_size), 1, output_file);
    
    // д��payload
    fwrite(payload_data, 1, payload_size, output_file);
    
    fclose(output_file);
    
    printf("�����!\n");
    printf("����ļ���С: %ld �ֽ�\n", stub_size + payload_size + sizeof(marker) + sizeof(payload_size));
    
    free(stub_data);
    free(payload_data);
    
    return 0;
}
EOF

# ����binder
$GCC64 $CFLAGS prop/binder_win8to11.c $LIBS -o enhanced_builds/binder_win8to11.exe

echo "3. ��������payload..."

# �����򵥵Ĳ���payload
cat << 'EOF' > prop/test_payload.c
#include <windows.h>

int main() {
    MessageBoxA(NULL, "Windows 8-11�����Բ��Գɹ�!", "����", MB_OK | MB_ICONINFORMATION);
    return 0;
}
EOF

$GCC64 $CFLAGS prop/test_payload.c $LIBS -o enhanced_builds/test_payload.exe

echo "4. ����ʹ��˵��..."

cat << 'EOF' > enhanced_builds/README.txt
=== Windows 8-11���ݹ���ʹ��˵�� ===

�ļ�˵��:
- stubloader_win8to11_x64.exe: 64λstubloader
- stubloader_win8to11_x86.exe: 32λstubloader  
- binder_win8to11.exe: �ļ��󶨹���
- test_payload.exe: ������payload

ʹ�ò���:
1. ʹ��binder��stubloader��payload:
   ./binder_win8to11.exe stubloader_win8to11_x64.exe test_payload.exe output.exe

2. ��Windows����������output.exe���в���

������˵��:
- Windows 8: ��ȫ����
- Windows 8.1: ��ȫ����
- Windows 10: ���ݣ�ע�ⰲȫ�������
- Windows 11: ���ݣ�������Ҫ����ԱȨ��

ע������:
- ������������в���
- ĳЩɱ�����������
- Windows 10/11�����Թ���ԱȨ������
EOF

echo "5. ���Ա�����..."

# ���԰󶨹���
if [ -f "enhanced_builds/binder_win8to11.exe" ] && [ -f "enhanced_builds/stubloader_win8to11_x64.exe" ] && [ -f "enhanced_builds/test_payload.exe" ]; then
    echo "�������԰��ļ�..."
    cd enhanced_builds
    wine binder_win8to11.exe stubloader_win8to11_x64.exe test_payload.exe test_output.exe 2>/dev/null || echo "Wine��������"
    cd ..
fi

echo "6. �������!"
echo
echo "����ļ�:"
ls -la enhanced_builds/

echo
echo "�ɹ�������ļ�:"
find enhanced_builds -name "*.exe" -exec basename {} \;

echo
echo "��һ��:"
echo "1. ��enhanced_buildsĿ¼���Ƶ�Windows����"
echo "2. �Ķ�README.txt�˽�ʹ�÷���"
echo "3. �ڲ�ͬWindows�汾�в��Լ�����"
