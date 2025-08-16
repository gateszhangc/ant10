#ifndef NET_UTIL_H
#define NET_UTIL_H

#include <winsock2.h>

// �����������ݣ�ȷ������ָ�����ȵ����ݣ�
int send_all(SOCKET sock, const void *buf, size_t len);

// �����������ݣ�ȷ������ָ�����ȵ����ݣ�
int recv_all(SOCKET sock, void *buf, size_t len);

// ����һ�����ݣ���CRLFΪ�ָ���������HTTPͷ����
int recv_line(SOCKET sock, char *buf, size_t size);

// ����HTTPͷ����ֱ���������У�
int recv_http_headers(SOCKET sock, char *buffer, size_t buf_size, size_t *header_len);

// ���ʮ������ת����������
void hex_dump(const char *data, size_t size);

#endif // NET_UTIL_H