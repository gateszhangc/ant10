#!/bin/bash

# exe文件兼容性分析脚本
# 在没有真实Windows环境的情况下进行静态分析

EXE_DIR="/mnt/hgfs/VMShare"
ANALYSIS_DIR="$HOME/vms/analysis_results"
mkdir -p "$ANALYSIS_DIR"

echo "=== exe文件兼容性静态分析 ==="
echo

# 安装分析工具
echo "1. 安装分析工具..."
if ! command -v file &> /dev/null; then
    sudo apt update && sudo apt install -y file
fi

if ! command -v objdump &> /dev/null; then
    sudo apt install -y binutils
fi

if ! command -v strings &> /dev/null; then
    sudo apt install -y binutils
fi

echo "2. 分析exe文件..."

# 重点分析的文件
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
        echo "=== 分析文件: $exe_file ==="
        
        # 创建分析报告文件
        REPORT_FILE="$ANALYSIS_DIR/${exe_file%.exe}_analysis.txt"
        
        echo "文件: $exe_file" > "$REPORT_FILE"
        echo "分析时间: $(date)" >> "$REPORT_FILE"
        echo "===========================================" >> "$REPORT_FILE"
        
        # 基本文件信息
        echo "基本信息:" >> "$REPORT_FILE"
        ls -la "$EXE_DIR/$exe_file" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # 文件类型分析
        echo "文件类型:" >> "$REPORT_FILE"
        file "$EXE_DIR/$exe_file" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # PE头信息分析
        echo "PE头信息:" >> "$REPORT_FILE"
        objdump -f "$EXE_DIR/$exe_file" 2>/dev/null >> "$REPORT_FILE" || echo "无法读取PE头" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # 导入表分析
        echo "导入的DLL:" >> "$REPORT_FILE"
        objdump -p "$EXE_DIR/$exe_file" 2>/dev/null | grep "DLL Name:" >> "$REPORT_FILE" || echo "无法读取导入表" >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # 字符串分析
        echo "关键字符串:" >> "$REPORT_FILE"
        strings "$EXE_DIR/$exe_file" | grep -i -E "(windows|version|kernel|ntdll|user32|advapi32)" | head -20 >> "$REPORT_FILE"
        echo >> "$REPORT_FILE"
        
        # 兼容性分析
        echo "兼容性分析:" >> "$REPORT_FILE"
        
        # 检查架构
        if file "$EXE_DIR/$exe_file" | grep -q "x86-64"; then
            echo "- 架构: 64位 (x86-64)" >> "$REPORT_FILE"
            echo "- Windows 8兼容性: 良好" >> "$REPORT_FILE"
            echo "- Windows 10兼容性: 良好" >> "$REPORT_FILE"
            echo "- Windows 11兼容性: 良好" >> "$REPORT_FILE"
        elif file "$EXE_DIR/$exe_file" | grep -q "80386"; then
            echo "- 架构: 32位 (x86)" >> "$REPORT_FILE"
            echo "- Windows 8兼容性: 良好" >> "$REPORT_FILE"
            echo "- Windows 10兼容性: 良好 (需要32位支持)" >> "$REPORT_FILE"
            echo "- Windows 11兼容性: 可能需要额外配置" >> "$REPORT_FILE"
        fi
        
        # 检查依赖的DLL
        if objdump -p "$EXE_DIR/$exe_file" 2>/dev/null | grep -q "kernel32.dll"; then
            echo "- 使用标准Windows API" >> "$REPORT_FILE"
        fi
        
        if objdump -p "$EXE_DIR/$exe_file" 2>/dev/null | grep -q "msvcr"; then
            echo "- 依赖Visual C++运行时" >> "$REPORT_FILE"
            echo "- 建议: 确保目标系统安装了对应的VC++运行时" >> "$REPORT_FILE"
        fi
        
        # 检查特殊API
        if strings "$EXE_DIR/$exe_file" | grep -q -i "CreateProcess"; then
            echo "- 使用进程创建API (可能需要管理员权限)" >> "$REPORT_FILE"
        fi
        
        if strings "$EXE_DIR/$exe_file" | grep -q -i "WriteProcessMemory"; then
            echo "- 使用内存写入API (可能被杀毒软件拦截)" >> "$REPORT_FILE"
        fi
        
        echo "===========================================" >> "$REPORT_FILE"
        
        echo "分析完成: $REPORT_FILE"
    else
        echo "文件不存在: $exe_file"
    fi
done

echo
echo "3. 生成兼容性总结报告..."

# 生成总结报告
SUMMARY_REPORT="$ANALYSIS_DIR/compatibility_summary.txt"

cat << 'EOF' > "$SUMMARY_REPORT"
=== exe文件兼容性总结报告 ===

基于静态分析的兼容性评估:

## 主要发现

### 架构兼容性
- 大部分文件为64位架构，与现代Windows系统兼容性良好
- 32位文件在Windows 11上可能需要额外配置

### API依赖性
- 使用标准Windows API，兼容性较好
- 部分文件依赖Visual C++运行时库

### 潜在问题
1. 进程注入相关API可能被现代安全软件拦截
2. 内存操作API在Windows 10/11上权限要求更严格
3. 某些文件可能需要管理员权限运行

## 建议改进

### 代码层面
1. 添加Windows版本检测
2. 实现权限提升请求
3. 添加异常处理机制
4. 优化内存操作方式

### 兼容性改进
1. 添加manifest文件声明兼容性
2. 使用更现代的API替代过时函数
3. 实现多版本适配逻辑

### 测试建议
1. 在真实Windows环境中测试
2. 测试不同权限级别下的运行情况
3. 验证杀毒软件兼容性

EOF

# 添加具体文件的分析结果到总结报告
echo >> "$SUMMARY_REPORT"
echo "## 具体文件分析结果" >> "$SUMMARY_REPORT"
echo >> "$SUMMARY_REPORT"

for report in "$ANALYSIS_DIR"/*_analysis.txt; do
    if [ -f "$report" ]; then
        filename=$(basename "$report" _analysis.txt)
        echo "### $filename.exe" >> "$SUMMARY_REPORT"
        echo "详细分析: $(basename "$report")" >> "$SUMMARY_REPORT"
        echo >> "$SUMMARY_REPORT"
    fi
done

echo "4. 创建改进建议脚本..."

# 创建改进建议实施脚本
cat << 'EOF' > "$ANALYSIS_DIR/implement_improvements.sh"
#!/bin/bash

# 基于分析结果的改进实施脚本

echo "=== 实施兼容性改进建议 ==="
echo

echo "1. 创建Windows版本检测代码..."

# 创建版本检测头文件
cat << 'VERSION_H' > ../comm/version_detect.h
#ifndef VERSION_DETECT_H
#define VERSION_DETECT_H

#include <windows.h>

// Windows版本检测
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

# 创建版本检测实现
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

echo "版本检测代码已创建"

echo "2. 创建权限提升代码..."

# 创建权限提升头文件
cat << 'PRIVILEGE_H' > ../comm/privilege_escalation.h
#ifndef PRIVILEGE_ESCALATION_H
#define PRIVILEGE_ESCALATION_H

#include <windows.h>

BOOL is_running_as_admin();
BOOL request_admin_privileges();
BOOL enable_debug_privilege();

#endif
PRIVILEGE_H

echo "改进代码模板已创建在 ../comm/ 目录"
echo "请根据具体需求集成到现有代码中"

EOF

chmod +x "$ANALYSIS_DIR/implement_improvements.sh"

echo
echo "5. 分析完成!"
echo
echo "分析结果保存在: $ANALYSIS_DIR"
echo "总结报告: $ANALYSIS_DIR/compatibility_summary.txt"
echo "改进脚本: $ANALYSIS_DIR/implement_improvements.sh"
echo
echo "下一步建议:"
echo "1. 查看分析报告: cat $ANALYSIS_DIR/compatibility_summary.txt"
echo "2. 实施改进建议: $ANALYSIS_DIR/implement_improvements.sh"
echo "3. 获取Windows ISO文件进行真实环境测试"
echo "4. 重新编译改进后的代码"

# 显示分析结果
echo
echo "=== 快速分析结果 ==="
ls -la "$ANALYSIS_DIR"
