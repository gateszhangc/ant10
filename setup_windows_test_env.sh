#!/bin/bash

# Windows 8-11测试环境完整设置脚本

VM_DIR="$HOME/vms"
ISO_DIR="$VM_DIR/iso"
TEST_DIR="$VM_DIR/test_results"

echo "=== Windows 8-11 exe文件测试环境设置 ==="
echo

# 创建目录结构
mkdir -p "$VM_DIR"
mkdir -p "$ISO_DIR"
mkdir -p "$TEST_DIR"

echo "1. 创建多个Windows版本的虚拟机磁盘..."

# 创建Windows 8虚拟机
if [ ! -f "$VM_DIR/windows8.qcow2" ]; then
    echo "创建Windows 8虚拟机磁盘..."
    qemu-img create -f qcow2 "$VM_DIR/windows8.qcow2" 25G
fi

# 创建Windows 10虚拟机
if [ ! -f "$VM_DIR/windows10.qcow2" ]; then
    echo "创建Windows 10虚拟机磁盘..."
    qemu-img create -f qcow2 "$VM_DIR/windows10.qcow2" 30G
fi

# 创建Windows 11虚拟机
if [ ! -f "$VM_DIR/windows11.qcow2" ]; then
    echo "创建Windows 11虚拟机磁盘..."
    qemu-img create -f qcow2 "$VM_DIR/windows11.qcow2" 35G
fi

echo "2. 创建Windows版本特定的启动脚本..."

# Windows 8启动脚本
cat << 'EOF' > "$VM_DIR/start_win8.sh"
#!/bin/bash
VM_DIR="$HOME/vms"
echo "启动Windows 8测试环境..."
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
echo "Windows 8虚拟机已启动，VNC端口: 5901"
EOF

# Windows 10启动脚本
cat << 'EOF' > "$VM_DIR/start_win10.sh"
#!/bin/bash
VM_DIR="$HOME/vms"
echo "启动Windows 10测试环境..."
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
echo "Windows 10虚拟机已启动，VNC端口: 5902"
EOF

# Windows 11启动脚本
cat << 'EOF' > "$VM_DIR/start_win11.sh"
#!/bin/bash
VM_DIR="$HOME/vms"
echo "启动Windows 11测试环境..."
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
echo "Windows 11虚拟机已启动，VNC端口: 5903"
EOF

chmod +x "$VM_DIR/start_win8.sh"
chmod +x "$VM_DIR/start_win10.sh"
chmod +x "$VM_DIR/start_win11.sh"

echo "3. 创建测试自动化脚本..."

# 创建exe文件测试脚本
cat << 'EOF' > "$VM_DIR/test_exe_compatibility.sh"
#!/bin/bash

# exe文件兼容性测试脚本

TEST_DIR="$HOME/vms/test_results"
EXE_DIR="/mnt/hgfs/VMShare"

echo "=== exe文件兼容性测试 ==="
echo

# 检查测试文件
echo "可用的测试exe文件:"
if [ -d "$EXE_DIR" ]; then
    ls -la "$EXE_DIR"/*.exe 2>/dev/null | head -10
else
    echo "未找到测试文件目录: $EXE_DIR"
fi

echo
echo "测试流程:"
echo "1. 启动对应的Windows版本虚拟机"
echo "2. 通过VNC连接到虚拟机"
echo "3. 安装Windows系统"
echo "4. 将exe文件传输到Windows中"
echo "5. 测试exe文件运行情况"
echo "6. 记录测试结果"
echo

echo "Windows版本启动命令:"
echo "- Windows 8:  ~/vms/start_win8.sh  (VNC: 5901)"
echo "- Windows 10: ~/vms/start_win10.sh (VNC: 5902)"
echo "- Windows 11: ~/vms/start_win11.sh (VNC: 5903)"
echo

echo "VNC连接命令:"
echo "- Windows 8:  vncviewer localhost:5901"
echo "- Windows 10: vncviewer localhost:5902"
echo "- Windows 11: vncviewer localhost:5903"
echo

# 创建测试结果模板
mkdir -p "$TEST_DIR"
cat << 'TEMPLATE' > "$TEST_DIR/test_template.txt"
=== exe文件兼容性测试报告 ===

测试文件: [exe文件名]
测试日期: [日期]

Windows 8测试结果:
- 启动状态: [成功/失败]
- 运行状态: [正常/异常/崩溃]
- 错误信息: [如有]
- 功能测试: [描述]

Windows 10测试结果:
- 启动状态: [成功/失败]
- 运行状态: [正常/异常/崩溃]
- 错误信息: [如有]
- 功能测试: [描述]

Windows 11测试结果:
- 启动状态: [成功/失败]
- 运行状态: [正常/异常/崩溃]
- 错误信息: [如有]
- 功能测试: [描述]

兼容性总结:
- 最佳兼容版本: [Windows版本]
- 需要修复的问题: [列表]
- 建议改进: [建议]
TEMPLATE

echo "测试结果模板已创建: $TEST_DIR/test_template.txt"
EOF

chmod +x "$VM_DIR/test_exe_compatibility.sh"

echo "4. 创建ISO文件获取指南..."

cat << 'EOF' > "$ISO_DIR/README.md"
# Windows ISO文件获取指南

## 官方下载地址

### Windows 8.1
- 官方地址: https://www.microsoft.com/zh-cn/software-download/windows8ISO
- 文件名: windows8.iso
- 大小: 约4GB

### Windows 10
- 官方地址: https://www.microsoft.com/zh-cn/software-download/windows10ISO
- 文件名: windows10.iso
- 大小: 约5GB

### Windows 11
- 官方地址: https://www.microsoft.com/zh-cn/software-download/windows11
- 文件名: windows11.iso
- 大小: 约5GB

## 下载说明

1. 访问官方网站
2. 选择对应的Windows版本
3. 下载ISO文件
4. 将ISO文件重命名并放置到此目录

## 文件放置

请将下载的ISO文件按以下命名放置到此目录:
- windows8.iso
- windows10.iso  
- windows11.iso

## 验证文件

下载完成后可以使用以下命令验证文件完整性:
```bash
ls -la *.iso
file *.iso
```
EOF

echo "5. 环境设置完成!"
echo
echo "虚拟机磁盘文件:"
ls -la "$VM_DIR"/*.qcow2 2>/dev/null

echo
echo "启动脚本:"
ls -la "$VM_DIR"/start_win*.sh 2>/dev/null

echo
echo "下一步操作:"
echo "1. 下载Windows ISO文件到: $ISO_DIR"
echo "2. 运行测试脚本: $VM_DIR/test_exe_compatibility.sh"
echo "3. 启动对应的Windows版本进行测试"
echo
echo "测试结果将保存到: $TEST_DIR"
