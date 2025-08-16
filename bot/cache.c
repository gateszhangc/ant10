#include "bot.h"

// 只保留一组变量定义
CachedCommand command_cache[MAX_CACHED_COMMANDS] = {0};
int command_cache_count = 0;

void cache_command(const char *cmd) {
    if (!cmd || command_cache_count >= MAX_CACHED_COMMANDS) {
        #ifdef DEBUG_MODE
        printf("[WARN] Command cache full or invalid command\n");
        #endif
        return;
    }
    
    // 复制命令到缓存
    command_cache[command_cache_count].command = _strdup(cmd);
    if (!command_cache[command_cache_count].command) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for command cache\n");
        #endif
        return;
    }
    
    // 添加时间戳
    command_cache[command_cache_count].timestamp = time(NULL);
    command_cache_count++;
    
    #ifdef DEBUG_MODE
    printf("[DEBUG] Cached command: %.*s...\n", 50, cmd);
    #endif
}

char* get_cached_command() {
    if (command_cache_count <= 0) {
        return NULL;
    }
    
    // 取出队列中的第一个命令
    char *cmd = command_cache[0].command;
    
    // 向前移动队列
    for (int i = 0; i < command_cache_count - 1; i++) {
        command_cache[i] = command_cache[i + 1];
    }
    
    command_cache_count--;
    command_cache[command_cache_count].command = NULL;
    command_cache[command_cache_count].timestamp = 0;
    
    return cmd;
}

void free_command_cache() {
    for (int i = 0; i < command_cache_count; i++) {
        if (command_cache[i].command) {
            free(command_cache[i].command);
            command_cache[i].command = NULL;
        }
    }
    command_cache_count = 0;
}