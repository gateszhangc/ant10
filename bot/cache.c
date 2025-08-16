#include "bot.h"

// ֻ����һ���������
CachedCommand command_cache[MAX_CACHED_COMMANDS] = {0};
int command_cache_count = 0;

void cache_command(const char *cmd) {
    if (!cmd || command_cache_count >= MAX_CACHED_COMMANDS) {
        #ifdef DEBUG_MODE
        printf("[WARN] Command cache full or invalid command\n");
        #endif
        return;
    }
    
    // �����������
    command_cache[command_cache_count].command = _strdup(cmd);
    if (!command_cache[command_cache_count].command) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Memory allocation failed for command cache\n");
        #endif
        return;
    }
    
    // ���ʱ���
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
    
    // ȡ�������еĵ�һ������
    char *cmd = command_cache[0].command;
    
    // ��ǰ�ƶ�����
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