#ifndef NET_UTIL_H
#define NET_UTIL_H

#include <winsock2.h>

// 发送完整数据（确保发送指定长度的数据）
int send_all(SOCKET sock, const void *buf, size_t len);

// 接收完整数据（确保接收指定长度的数据）
int recv_all(SOCKET sock, void *buf, size_t len);

// 接收一行数据（以CRLF为分隔符，用于HTTP头部）
int recv_line(SOCKET sock, char *buf, size_t size);

// 接收HTTP头部（直到遇到空行）
int recv_http_headers(SOCKET sock, char *buffer, size_t buf_size, size_t *header_len);

// 添加十六进制转储函数声明
void hex_dump(const char *data, size_t size);

#endif // NET_UTIL_H