// comm/crypto.h
#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stdlib.h>

// ԭʼ��������
void aes_encrypt(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *key);
void aes_decrypt(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *key);
void key_expansion(const uint8_t *key, uint32_t *w);

// ��Ӵ����ĺ�������
void aes_encrypt_padded(const uint8_t *input, uint8_t *output, size_t *len, const uint8_t *key);
void aes_decrypt_padded(const uint8_t *input, uint8_t *output, size_t *len, const uint8_t *key);

void safe_print(const char* message);

#endif // CRYPTO_H