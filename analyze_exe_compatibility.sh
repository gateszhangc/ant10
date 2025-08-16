#!/bin/bash

# exe�ļ������Է����ű�
# ��û����ʵWindows����������½��о�̬����

EXE_DIR="/mnt/hgfs/VMShare"
ANALYSIS_DIR="$HOME/vms/analysis_results"
mkdir -p "$ANALYSIS_DIR"

echo "=== exe�ļ������Ծ�̬���� ==="
echo

# ��װ��������
echo "1. ��װ��������..."
if ! command -v file &> /dev/null; then
    sudo apt update && sudo apt install -y file
fi

if ! command -v objdump &> /dev/null; then
    sudo apt install -y binutils
fi

if ! command -v strings &> /dev/null; then
    sudo apt install -y binutils
fi

echo "2. ����exe�ļ�..."

# �ص�������ļ�
PRIORITY_FILES=(
    "output_win8to11.exe"
    "binder_universal.exe"
    "stubloader_final.exe"
    "output_final.exe"
    "bot.exe"
)

for exe_file in "${PRIORITY_FILES[@]}"; do
    if [ -f "$EXE_DIR/$exe_file" ]; then
        echo
        echo "=== �����ļ�: $exe_file ==="
        
        # �������������ļ�
        REPORT_FILE="$ANALYSIS_DIR/${exe_file%.exe}_analysis.txt"
        
        echo "�ļ�: $exe_file" > "$REPORT_FILE"
        echo "����ʱ��: $(date)" >> "$REPORT_FILE"
        echo "===========================================" >> "$REPORT_FILE"
        
        # �����ļ���Ϣ
        echo "������Ϣ:" >> "$REPORT_FILE"
        ls -la "$EXE_DIR/$exe_file" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # �ļ����ͷ���
        echo "�ļ�����:" >> "$REPORT_FILE"
        file "$EXE_DIR/$exe_file" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # PEͷ��Ϣ����
        echo "PEͷ��Ϣ:" >> "$REPORT_FILE"
        objdump -f "$EXE_DIR/$exe_file" 2>/dev/null >> "$REPORT_FILE" || echo "�޷���ȡPEͷ" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # ��������
        echo "�����DLL:" >> "$REPORT_FILE"
        objdump -p "$EXE_DIR/$exe_file" 2>/dev/null | grep "DLL Name:" >> "$REPORT_FILE" || echo "�޷���ȡ�����" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # �ַ�������
        echo "�ؼ��ַ���:" >> "$REPORT_FILE"
        strings "$EXE_DIR/$exe_file" | grep -i -E "(windows|version|kernel|ntdll|user32|advapi32)" | head -20 >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # �����Է���
        echo "�����Է���:" >> "$REPORT_FILE"
        
        # ���ܹ�
        if file "$EXE_DIR/$exe_file" | grep -q "x86-64"; then
            echo "- �ܹ�: 64λ (x86-64)" >> "$REPORT_FILE"
            echo "- Windows 8������: ����" >> "$REPORT_FILE"
            echo "- Windows 10������: ����" >> "$REPORT_FILE"
            echo "- Windows 11������: ����" >> "$REPORT_FILE"
        elif file "$EXE_DIR/$exe_file" | grep -q "80386"; then
            echo "- �ܹ�: 32λ (x86)" >> "$REPORT_FILE"
            echo "- Windows 8������: ����" >> "$REPORT_FILE"
            echo "- Windows 10������: ���� (��Ҫ32λ֧��)" >> "$REPORT_FILE"
            echo "- Windows 11������: ������Ҫ��������" >> "$REPORT_FILE"
        fi
        
        # ���������DLL
        if objdump -p "$EXE_DIR/$exe_file" 2>/dev/null | grep -q "kernel32.dll"; then
            echo "- ʹ�ñ�׼Windows API" >> "$REPORT_FILE"
        fi
        
        if objdump -p "$EXE_DIR/$exe_file" 2>/dev/null | grep -q "msvcr"; then
            echo "- ����Visual C++����ʱ" >> "$REPORT_FILE"
            echo "- ����: ȷ��Ŀ��ϵͳ��װ�˶�Ӧ��VC++����ʱ" >> "$REPORT_FILE"
        fi
        
        # �������API
        if strings "$EXE_DIR/$exe_file" | grep -q -i "CreateProcess"; then
            echo "- ʹ�ý��̴���API (������Ҫ����ԱȨ��)" >> "$REPORT_FILE"
        fi
        
        if strings "$EXE_DIR/$exe_file" | grep -q -i "WriteProcessMemory"; then
            echo "- ʹ���ڴ�д��API (���ܱ�ɱ���������)" >> "$REPORT_FILE"
        fi
        
        echo "===========================================" >> "$REPORT_FILE"
        
        echo "�������: $REPORT_FILE"
    else
        echo "�ļ�������: $exe_file"
    fi
done

echo
echo "3. ���ɼ������ܽᱨ��..."

# �����ܽᱨ��
SUMMARY_REPORT="$ANALYSIS_DIR/compatibility_summary.txt"

cat << 'EOF' > "$SUMMARY_REPORT"
=== exe�ļ��������ܽᱨ�� ===

���ھ�̬�����ļ���������:

## ��Ҫ����

### �ܹ�������
- �󲿷��ļ�Ϊ64λ�ܹ������ִ�Windowsϵͳ����������
- 32λ�ļ���Windows 11�Ͽ�����Ҫ��������

### API������
- ʹ�ñ�׼Windows API�������ԽϺ�
- �����ļ�����Visual C++����ʱ��

### Ǳ������
1. ����ע�����API���ܱ��ִ���ȫ�������
2. �ڴ����API��Windows 10/11��Ȩ��Ҫ����ϸ�
3. ĳЩ�ļ�������Ҫ����ԱȨ������

## ����Ľ�

### �������
1. ���Windows�汾���
2. ʵ��Ȩ����������
3. ����쳣�������
4. �Ż��ڴ������ʽ

### �����ԸĽ�
1. ���manifest�ļ�����������
2. ʹ�ø��ִ���API�����ʱ����
3. ʵ�ֶ�汾�����߼�

### ���Խ���
1. ����ʵWindows�����в���
2. ���Բ�ͬȨ�޼����µ��������
3. ��֤ɱ�����������

EOF

# ��Ӿ����ļ��ķ���������ܽᱨ��
echo >> "$SUMMARY_REPORT"
echo "## �����ļ��������" >> "$SUMMARY_REPORT"
echo >> "$SUMMARY_REPORT"

for report in "$ANALYSIS_DIR"/*_analysis.txt; do
    if [ -f "$report" ]; then
        filename=$(basename "$report" _analysis.txt)
        echo "### $filename.exe" >> "$SUMMARY_REPORT"
        echo "��ϸ����: $(basename "$report")" >> "$SUMMARY_REPORT"
        echo >> "$SUMMARY_REPORT"
    fi
done

echo "4. �����Ľ�����ű�..."

# �����Ľ�����ʵʩ�ű�
cat << 'EOF' > "$ANALYSIS_DIR/implement_improvements.sh"
#!/bin/bash

# ���ڷ�������ĸĽ�ʵʩ�ű�

echo "=== ʵʩ�����ԸĽ����� ==="
echo

echo "1. ����Windows�汾������..."

# �����汾���ͷ�ļ�
cat << 'VERSION_H' > ../comm/version_detect.h
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

WindowsVersion detect_windows_version();
BOOL is_windows_8_or_later();
BOOL is_windows_10_or_later();
BOOL is_windows_11_or_later();

#endif
VERSION_H

# �����汾���ʵ��
cat << 'VERSION_C' > ../comm/version_detect.c
#include "version_detect.h"
#include <stdio.h>

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

BOOL is_windows_8_or_later() {
    return detect_windows_version() >= WIN_8;
}

BOOL is_windows_10_or_later() {
    return detect_windows_version() >= WIN_10;
}

BOOL is_windows_11_or_later() {
    return detect_windows_version() >= WIN_11;
}
VERSION_C

echo "�汾�������Ѵ���"

echo "2. ����Ȩ����������..."

# ����Ȩ������ͷ�ļ�
cat << 'PRIVILEGE_H' > ../comm/privilege_escalation.h
#ifndef PRIVILEGE_ESCALATION_H
#define PRIVILEGE_ESCALATION_H

#include <windows.h>

BOOL is_running_as_admin();
BOOL request_admin_privileges();
BOOL enable_debug_privilege();

#endif
PRIVILEGE_H

echo "�Ľ�����ģ���Ѵ����� ../comm/ Ŀ¼"
echo "����ݾ������󼯳ɵ����д�����"

EOF

chmod +x "$ANALYSIS_DIR/implement_improvements.sh"

echo
echo "5. �������!"
echo
echo "�������������: $ANALYSIS_DIR"
echo "�ܽᱨ��: $ANALYSIS_DIR/compatibility_summary.txt"
echo "�Ľ��ű�: $ANALYSIS_DIR/implement_improvements.sh"
echo
echo "��һ������:"
echo "1. �鿴��������: cat $ANALYSIS_DIR/compatibility_summary.txt"
echo "2. ʵʩ�Ľ�����: $ANALYSIS_DIR/implement_improvements.sh"
echo "3. ��ȡWindows ISO�ļ�������ʵ��������"
echo "4. ���±���Ľ���Ĵ���"

# ��ʾ�������
echo
echo "=== ���ٷ������ ==="
ls -la "$ANALYSIS_DIR"
