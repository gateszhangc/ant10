#!/bin/bash

# Windows 8-11���Ի����������ýű�

VM_DIR="$HOME/vms"
ISO_DIR="$VM_DIR/iso"
TEST_DIR="$VM_DIR/test_results"

echo "=== Windows 8-11 exe�ļ����Ի������� ==="
echo

# ����Ŀ¼�ṹ
mkdir -p "$VM_DIR"
mkdir -p "$ISO_DIR"
mkdir -p "$TEST_DIR"

echo "1. �������Windows�汾�����������..."

# ����Windows 8�����
if [ ! -f "$VM_DIR/windows8.qcow2" ]; then
    echo "����Windows 8���������..."
    qemu-img create -f qcow2 "$VM_DIR/windows8.qcow2" 25G
fi

# ����Windows 10�����
if [ ! -f "$VM_DIR/windows10.qcow2" ]; then
    echo "����Windows 10���������..."
    qemu-img create -f qcow2 "$VM_DIR/windows10.qcow2" 30G
fi

# ����Windows 11�����
if [ ! -f "$VM_DIR/windows11.qcow2" ]; then
    echo "����Windows 11���������..."
    qemu-img create -f qcow2 "$VM_DIR/windows11.qcow2" 35G
fi

echo "2. ����Windows�汾�ض��������ű�..."

# Windows 8�����ű�
cat << 'EOF' > "$VM_DIR/start_win8.sh"
#!/bin/bash
VM_DIR="$HOME/vms"
echo "����Windows 8���Ի���..."
qemu-system-x86_64 \
    -m 2048 \
    -hda "$VM_DIR/windows8.qcow2" \
    -cdrom "$VM_DIR/iso/windows8.iso" \
    -boot d \
    -vnc :1 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device rtl8139,netdev=net0 \
    -usb -device usb-tablet \
    -enable-kvm 2>/dev/null || \
qemu-system-x86_64 \
    -m 2048 \
    -hda "$VM_DIR/windows8.qcow2" \
    -cdrom "$VM_DIR/iso/windows8.iso" \
    -boot d \
    -vnc :1 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device rtl8139,netdev=net0 \
    -usb -device usb-tablet &
echo "Windows 8�������������VNC�˿�: 5901"
EOF

# Windows 10�����ű�
cat << 'EOF' > "$VM_DIR/start_win10.sh"
#!/bin/bash
VM_DIR="$HOME/vms"
echo "����Windows 10���Ի���..."
qemu-system-x86_64 \
    -m 3072 \
    -hda "$VM_DIR/windows10.qcow2" \
    -cdrom "$VM_DIR/iso/windows10.iso" \
    -boot d \
    -vnc :2 \
    -netdev user,id=net0,hostfwd=tcp::2223-:22 \
    -device rtl8139,netdev=net0 \
    -usb -device usb-tablet \
    -enable-kvm 2>/dev/null || \
qemu-system-x86_64 \
    -m 3072 \
    -hda "$VM_DIR/windows10.qcow2" \
    -cdrom "$VM_DIR/iso/windows10.iso" \
    -boot d \
    -vnc :2 \
    -netdev user,id=net0,hostfwd=tcp::2223-:22 \
    -device rtl8139,netdev=net0 \
    -usb -device usb-tablet &
echo "Windows 10�������������VNC�˿�: 5902"
EOF

# Windows 11�����ű�
cat << 'EOF' > "$VM_DIR/start_win11.sh"
#!/bin/bash
VM_DIR="$HOME/vms"
echo "����Windows 11���Ի���..."
qemu-system-x86_64 \
    -m 4096 \
    -hda "$VM_DIR/windows11.qcow2" \
    -cdrom "$VM_DIR/iso/windows11.iso" \
    -boot d \
    -vnc :3 \
    -netdev user,id=net0,hostfwd=tcp::2224-:22 \
    -device rtl8139,netdev=net0 \
    -usb -device usb-tablet \
    -machine q35 \
    -cpu host \
    -enable-kvm 2>/dev/null || \
qemu-system-x86_64 \
    -m 4096 \
    -hda "$VM_DIR/windows11.qcow2" \
    -cdrom "$VM_DIR/iso/windows11.iso" \
    -boot d \
    -vnc :3 \
    -netdev user,id=net0,hostfwd=tcp::2224-:22 \
    -device rtl8139,netdev=net0 \
    -usb -device usb-tablet \
    -machine q35 &
echo "Windows 11�������������VNC�˿�: 5903"
EOF

chmod +x "$VM_DIR/start_win8.sh"
chmod +x "$VM_DIR/start_win10.sh"
chmod +x "$VM_DIR/start_win11.sh"

echo "3. ���������Զ����ű�..."

# ����exe�ļ����Խű�
cat << 'EOF' > "$VM_DIR/test_exe_compatibility.sh"
#!/bin/bash

# exe�ļ������Բ��Խű�

TEST_DIR="$HOME/vms/test_results"
EXE_DIR="/mnt/hgfs/VMShare"

echo "=== exe�ļ������Բ��� ==="
echo

# �������ļ�
echo "���õĲ���exe�ļ�:"
if [ -d "$EXE_DIR" ]; then
    ls -la "$EXE_DIR"/*.exe 2>/dev/null | head -10
else
    echo "δ�ҵ������ļ�Ŀ¼: $EXE_DIR"
fi

echo
echo "��������:"
echo "1. ������Ӧ��Windows�汾�����"
echo "2. ͨ��VNC���ӵ������"
echo "3. ��װWindowsϵͳ"
echo "4. ��exe�ļ����䵽Windows��"
echo "5. ����exe�ļ��������"
echo "6. ��¼���Խ��"
echo

echo "Windows�汾��������:"
echo "- Windows 8:  ~/vms/start_win8.sh  (VNC: 5901)"
echo "- Windows 10: ~/vms/start_win10.sh (VNC: 5902)"
echo "- Windows 11: ~/vms/start_win11.sh (VNC: 5903)"
echo

echo "VNC��������:"
echo "- Windows 8:  vncviewer localhost:5901"
echo "- Windows 10: vncviewer localhost:5902"
echo "- Windows 11: vncviewer localhost:5903"
echo

# �������Խ��ģ��
mkdir -p "$TEST_DIR"
cat << 'TEMPLATE' > "$TEST_DIR/test_template.txt"
=== exe�ļ������Բ��Ա��� ===

�����ļ�: [exe�ļ���]
��������: [����]

Windows 8���Խ��:
- ����״̬: [�ɹ�/ʧ��]
- ����״̬: [����/�쳣/����]
- ������Ϣ: [����]
- ���ܲ���: [����]

Windows 10���Խ��:
- ����״̬: [�ɹ�/ʧ��]
- ����״̬: [����/�쳣/����]
- ������Ϣ: [����]
- ���ܲ���: [����]

Windows 11���Խ��:
- ����״̬: [�ɹ�/ʧ��]
- ����״̬: [����/�쳣/����]
- ������Ϣ: [����]
- ���ܲ���: [����]

�������ܽ�:
- ��Ѽ��ݰ汾: [Windows�汾]
- ��Ҫ�޸�������: [�б�]
- ����Ľ�: [����]
TEMPLATE

echo "���Խ��ģ���Ѵ���: $TEST_DIR/test_template.txt"
EOF

chmod +x "$VM_DIR/test_exe_compatibility.sh"

echo "4. ����ISO�ļ���ȡָ��..."

cat << 'EOF' > "$ISO_DIR/README.md"
# Windows ISO�ļ���ȡָ��

## �ٷ����ص�ַ

### Windows 8.1
- �ٷ���ַ: https://www.microsoft.com/zh-cn/software-download/windows8ISO
- �ļ���: windows8.iso
- ��С: Լ4GB

### Windows 10
- �ٷ���ַ: https://www.microsoft.com/zh-cn/software-download/windows10ISO
- �ļ���: windows10.iso
- ��С: Լ5GB

### Windows 11
- �ٷ���ַ: https://www.microsoft.com/zh-cn/software-download/windows11
- �ļ���: windows11.iso
- ��С: Լ5GB

## ����˵��

1. ���ʹٷ���վ
2. ѡ���Ӧ��Windows�汾
3. ����ISO�ļ�
4. ��ISO�ļ������������õ���Ŀ¼

## �ļ�����

�뽫���ص�ISO�ļ��������������õ���Ŀ¼:
- windows8.iso
- windows10.iso  
- windows11.iso

## ��֤�ļ�

������ɺ����ʹ������������֤�ļ�������:
```bash
ls -la *.iso
file *.iso
```
EOF

echo "5. �����������!"
echo
echo "����������ļ�:"
ls -la "$VM_DIR"/*.qcow2 2>/dev/null

echo
echo "�����ű�:"
ls -la "$VM_DIR"/start_win*.sh 2>/dev/null

echo
echo "��һ������:"
echo "1. ����Windows ISO�ļ���: $ISO_DIR"
echo "2. ���в��Խű�: $VM_DIR/test_exe_compatibility.sh"
echo "3. ������Ӧ��Windows�汾���в���"
echo
echo "���Խ�������浽: $TEST_DIR"
