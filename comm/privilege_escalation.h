#ifndef PRIVILEGE_ESCALATION_H
#define PRIVILEGE_ESCALATION_H

#include <windows.h>

// 权限检查和提升函数
BOOL is_running_as_admin();
BOOL request_admin_privileges();
BOOL enable_debug_privilege();
BOOL enable_privilege(LPCWSTR privilege_name);

// UAC相关函数
BOOL is_uac_enabled();
BOOL bypass_uac_if_possible();

// 进程权限相关
BOOL adjust_process_privileges();
BOOL check_required_privileges();

#endif // PRIVILEGE_ESCALATION_H
