#ifndef PRIVILEGE_ESCALATION_H
#define PRIVILEGE_ESCALATION_H

#include <windows.h>

// Ȩ�޼�����������
BOOL is_running_as_admin();
BOOL request_admin_privileges();
BOOL enable_debug_privilege();
BOOL enable_privilege(LPCWSTR privilege_name);

// UAC��غ���
BOOL is_uac_enabled();
BOOL bypass_uac_if_possible();

// ����Ȩ�����
BOOL adjust_process_privileges();
BOOL check_required_privileges();

#endif // PRIVILEGE_ESCALATION_H
