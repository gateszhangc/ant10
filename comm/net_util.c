#include "net_util.h"
#include <string.h>
#include <stdio.h>  // 添加必要的头文件

// 发送完整数据
int send_all(SOCKET sock, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char*)buf;
    
    while (total_sent < len) {
        int sent = send(sock, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1; // 发送失败
        }
        total_sent += sent;
    }
    return total_sent; // 返回实际发送的字节数
}

// 接收完整数据
int recv_all(SOCKET sock, void *buf, size_t len) {
    size_t total_received = 0;
    char *ptr = (char*)buf;
    
    while (total_received < len) {
        int received = recv(sock, ptr + total_received, len - total_received, 0);
        if (received <= 0) {
            return -1; // 接收失败
        }
        total_received += received;
    }
    return total_received; // 返回实际接收的字节数
}

// 文档14（网络工具）中的 recv_line 完整实现
int recv_line(SOCKET sock, char *buf, size_t size) {
    size_t i = 0;
    char c;
    
    // 确保缓冲区足够容纳结束符
    if (size < 2) return -1;
    
    while (i < size - 1) {
        // 每次接收一个字节
        int bytes_received = recv(sock, &c, 1, 0);
        
        // 检查接收状态
        if (bytes_received <= 0) {
            // 连接关闭或出错
            if (i == 0) return -1; // 没有收到任何数据
            break; // 部分数据有效
        }
        
        // 处理换行符（CR和LF）
        if (c == '\n') {
            // 处理LF换行符（Unix风格）
            break;
        } else if (c == '\r') {
            // 处理CR或CRLF换行符
            // 检查下一个字符是否是LF（但不从流中移除）
            char next;
            int peek_bytes = recv(sock, &next, 1, MSG_PEEK);
            
            if (peek_bytes > 0 && next == '\n') {
                // 是CRLF，消耗LF字符
                recv(sock, &next, 1, 0);
            }
            break;
        }
        
        // 存储普通字符
        buf[i++] = c;
    }
    
    // 添加字符串结束符
    buf[i] = '\0';
    
    // 返回接收的字节数（不包括结束符）
    return i;
}

int recv_http_headers(SOCKET sock, char *buffer, size_t buf_size, size_t *header_len) {
    char *ptr = buffer;
    size_t received = 0;
    const char *end_marker = "\r\n\r\n";
    size_t marker_len = 4;
    size_t marker_pos = 0;
    
    // 确保有足够空间存储结束符
    if (buf_size < marker_len + 1) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Buffer too small for HTTP headers\n");
        #endif
        return -1;
    }
    
    while (received < buf_size - 1) {
        // 尝试接收一个字节
        int bytes = recv(sock, ptr, 1, 0);
        if (bytes <= 0) {
            if (received == 0) {
                #ifdef DEBUG_MODE
                printf("[ERROR] Connection closed before headers\n");
                #endif
                return -2; // 连接关闭
            }
            break; // 部分数据有效
        }
        
        // 更新接收计数
        received++;
        
        // 检查是否匹配结束标记
        if (*ptr == end_marker[marker_pos]) {
            marker_pos++;
            if (marker_pos == marker_len) {
                // 找到完整结束标记
                *header_len = received;
                
                // 添加终止符以便安全打印
                buffer[received] = '\0';
                
                #ifdef DEBUG_MODE
                printf("[DEBUG] Found header end at %zu bytes\n", received);
                #endif
                
                return 0;
            }
        } else {
            // 重置标记匹配位置
            marker_pos = 0;
        }
        
        ptr++;
    }
    
    #ifdef DEBUG_MODE
    printf("[WARN] Header end marker not found in %zu bytes\n", received);
    #endif
    
    return -1; // 未找到结束标记
}

// 添加十六进制转储函数实现
void hex_dump(const char *data, size_t size) {
    const int bytes_per_line = 16;
    for (size_t i = 0; i < size; i += bytes_per_line) {
        // 打印偏移量
        printf("%04zX: ", i);
        
        // 打印十六进制
        for (int j = 0; j < bytes_per_line; j++) {
            if (i + j < size) {
                printf("%02X ", (unsigned char)data[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" "); // 中间分隔符
        }
        
        printf(" ");
        
        // 打印ASCII
        for (int j = 0; j < bytes_per_line; j++) {
            if (i + j < size) {
                unsigned char c = data[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            } else {
                printf(" ");
            }
        }
        
        printf("\n");
    }
}