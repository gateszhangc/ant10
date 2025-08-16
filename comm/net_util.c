#include "net_util.h"
#include <string.h>
#include <stdio.h>  // ��ӱ�Ҫ��ͷ�ļ�

// ������������
int send_all(SOCKET sock, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char*)buf;
    
    while (total_sent < len) {
        int sent = send(sock, ptr + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            return -1; // ����ʧ��
        }
        total_sent += sent;
    }
    return total_sent; // ����ʵ�ʷ��͵��ֽ���
}

// ������������
int recv_all(SOCKET sock, void *buf, size_t len) {
    size_t total_received = 0;
    char *ptr = (char*)buf;
    
    while (total_received < len) {
        int received = recv(sock, ptr + total_received, len - total_received, 0);
        if (received <= 0) {
            return -1; // ����ʧ��
        }
        total_received += received;
    }
    return total_received; // ����ʵ�ʽ��յ��ֽ���
}

// �ĵ�14�����繤�ߣ��е� recv_line ����ʵ��
int recv_line(SOCKET sock, char *buf, size_t size) {
    size_t i = 0;
    char c;
    
    // ȷ���������㹻���ɽ�����
    if (size < 2) return -1;
    
    while (i < size - 1) {
        // ÿ�ν���һ���ֽ�
        int bytes_received = recv(sock, &c, 1, 0);
        
        // ������״̬
        if (bytes_received <= 0) {
            // ���ӹرջ����
            if (i == 0) return -1; // û���յ��κ�����
            break; // ����������Ч
        }
        
        // �����з���CR��LF��
        if (c == '\n') {
            // ����LF���з���Unix���
            break;
        } else if (c == '\r') {
            // ����CR��CRLF���з�
            // �����һ���ַ��Ƿ���LF�������������Ƴ���
            char next;
            int peek_bytes = recv(sock, &next, 1, MSG_PEEK);
            
            if (peek_bytes > 0 && next == '\n') {
                // ��CRLF������LF�ַ�
                recv(sock, &next, 1, 0);
            }
            break;
        }
        
        // �洢��ͨ�ַ�
        buf[i++] = c;
    }
    
    // ����ַ���������
    buf[i] = '\0';
    
    // ���ؽ��յ��ֽ�������������������
    return i;
}

int recv_http_headers(SOCKET sock, char *buffer, size_t buf_size, size_t *header_len) {
    char *ptr = buffer;
    size_t received = 0;
    const char *end_marker = "\r\n\r\n";
    size_t marker_len = 4;
    size_t marker_pos = 0;
    
    // ȷ�����㹻�ռ�洢������
    if (buf_size < marker_len + 1) {
        #ifdef DEBUG_MODE
        printf("[ERROR] Buffer too small for HTTP headers\n");
        #endif
        return -1;
    }
    
    while (received < buf_size - 1) {
        // ���Խ���һ���ֽ�
        int bytes = recv(sock, ptr, 1, 0);
        if (bytes <= 0) {
            if (received == 0) {
                #ifdef DEBUG_MODE
                printf("[ERROR] Connection closed before headers\n");
                #endif
                return -2; // ���ӹر�
            }
            break; // ����������Ч
        }
        
        // ���½��ռ���
        received++;
        
        // ����Ƿ�ƥ��������
        if (*ptr == end_marker[marker_pos]) {
            marker_pos++;
            if (marker_pos == marker_len) {
                // �ҵ������������
                *header_len = received;
                
                // �����ֹ���Ա㰲ȫ��ӡ
                buffer[received] = '\0';
                
                #ifdef DEBUG_MODE
                printf("[DEBUG] Found header end at %zu bytes\n", received);
                #endif
                
                return 0;
            }
        } else {
            // ���ñ��ƥ��λ��
            marker_pos = 0;
        }
        
        ptr++;
    }
    
    #ifdef DEBUG_MODE
    printf("[WARN] Header end marker not found in %zu bytes\n", received);
    #endif
    
    return -1; // δ�ҵ��������
}

// ���ʮ������ת������ʵ��
void hex_dump(const char *data, size_t size) {
    const int bytes_per_line = 16;
    for (size_t i = 0; i < size; i += bytes_per_line) {
        // ��ӡƫ����
        printf("%04zX: ", i);
        
        // ��ӡʮ������
        for (int j = 0; j < bytes_per_line; j++) {
            if (i + j < size) {
                printf("%02X ", (unsigned char)data[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" "); // �м�ָ���
        }
        
        printf(" ");
        
        // ��ӡASCII
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