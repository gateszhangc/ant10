// security_checks.h
#ifndef SECURITY_CHECKS_H
#define SECURITY_CHECKS_H

#include <windows.h>

// �����Լ�⺯������ (ʹ���»�������)
BOOL anti_debug();

// �û���Ϊ��⺯������ (ʹ���»�������)
BOOL check_user_activity();

// �� security_checks.h ���
#define DYNCALL(func, ...) \
    ( (decltype(&func))GetProcAddress(LoadLibraryA( \
        CHAR_SPLIT('k','e','r','n','e','l','3','2','.','d','l','l',0) ), \
        CHAR_SPLIT(#func,0) ) ) (__VA_ARGS__)

// �ַ����ָ�꣨��������API��
#define CHAR_SPLIT(...) __VA_ARGS__


#endif // SECURITY_CHECKS_H