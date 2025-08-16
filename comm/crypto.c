#include "crypto.h"
#include <string.h>
#include <stdio.h>  

// AES常量
static const uint8_t sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

static const uint8_t inv_sbox[256] = {
    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
    0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
    0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
    0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
    0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
    0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
    0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
    0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
    0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
    0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
    0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
    0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
    0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
    0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
    0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};

static const uint8_t rcon[11] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x00
};

#define Nb 4
#define Nk 8  // AES-256使用8个32位字(256位)的密钥
#define Nr 14 // AES-256轮数

// 内部状态矩阵操作
static void sub_bytes(uint8_t state[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = sbox[state[i][j]];
        }
    }
}

static void inv_sub_bytes(uint8_t state[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = inv_sbox[state[i][j]];
        }
    }
}

static void shift_rows(uint8_t state[4][4]) {
    uint8_t temp;
    
    // 第二行左移1位
    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;
    
    // 第三行左移2位
    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;
    
    // 第四行左移3位
    temp = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = temp;
}

static void inv_shift_rows(uint8_t state[4][4]) {
    uint8_t temp;
    
    // 第二行右移1位
    temp = state[1][3];
    state[1][3] = state[1][2];
    state[1][2] = state[1][1];
    state[1][1] = state[1][0];
    state[1][0] = temp;
    
    // 第三行右移2位
    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;
    
    // 第四行右移3位
    temp = state[3][0];
    state[3][0] = state[3][1];
    state[3][1] = state[3][2];
    state[3][2] = state[3][3];
    state[3][3] = temp;
}

static uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    uint8_t hi_bit_set;
    for (int counter = 0; counter < 8; counter++) {
        if ((b & 1) == 1) p ^= a;
        hi_bit_set = (a & 0x80);
        a <<= 1;
        if (hi_bit_set == 0x80) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void mix_columns(uint8_t state[4][4]) {
    uint8_t a[4];
    
    for (int i = 0; i < 4; i++) {
        a[0] = state[0][i];
        a[1] = state[1][i];
        a[2] = state[2][i];
        a[3] = state[3][i];
        
        state[0][i] = gf_mul(0x02, a[0]) ^ gf_mul(0x03, a[1]) ^ a[2] ^ a[3];
        state[1][i] = a[0] ^ gf_mul(0x02, a[1]) ^ gf_mul(0x03, a[2]) ^ a[3];
        state[2][i] = a[0] ^ a[1] ^ gf_mul(0x02, a[2]) ^ gf_mul(0x03, a[3]);
        state[3][i] = gf_mul(0x03, a[0]) ^ a[1] ^ a[2] ^ gf_mul(0x02, a[3]);
    }
}

static void inv_mix_columns(uint8_t state[4][4]) {
    uint8_t a[4];
    
    for (int i = 0; i < 4; i++) {
        a[0] = state[0][i];
        a[1] = state[1][i];
        a[2] = state[2][i];
        a[3] = state[3][i];
        
        state[0][i] = gf_mul(0x0e, a[0]) ^ gf_mul(0x0b, a[1]) ^ gf_mul(0x0d, a[2]) ^ gf_mul(0x09, a[3]);
        state[1][i] = gf_mul(0x09, a[0]) ^ gf_mul(0x0e, a[1]) ^ gf_mul(0x0b, a[2]) ^ gf_mul(0x0d, a[3]);
        state[2][i] = gf_mul(0x0d, a[0]) ^ gf_mul(0x09, a[1]) ^ gf_mul(0x0e, a[2]) ^ gf_mul(0x0b, a[3]);
        state[3][i] = gf_mul(0x0b, a[0]) ^ gf_mul(0x0d, a[1]) ^ gf_mul(0x09, a[2]) ^ gf_mul(0x0e, a[3]);
    }
}

static void add_round_key(uint8_t state[4][4], const uint32_t *w, int round) {
    for (int c = 0; c < 4; c++) {
        uint32_t word = w[round * 4 + c];
        for (int r = 0; r < 4; r++) {
            state[r][c] ^= (word >> (24 - r * 8)) & 0xFF;
        }
    }
}

// 密钥扩展
void key_expansion(const uint8_t *key, uint32_t *w) {
    uint32_t temp;
    
    // 复制初始密钥
    for (int i = 0; i < Nk; i++) {
        w[i] = ((uint32_t)key[4*i] << 24) | 
               ((uint32_t)key[4*i+1] << 16) | 
               ((uint32_t)key[4*i+2] << 8) | 
               (uint32_t)key[4*i+3];
    }
    
    for (int i = Nk; i < Nb * (Nr + 1); i++) {
        temp = w[i-1];
        
        if (i % Nk == 0) {
            // 密钥调度核心
            uint32_t rot = (temp << 8) | (temp >> 24);
            uint32_t sub = 0;
            for (int j = 0; j < 4; j++) {
                sub |= (uint32_t)sbox[(rot >> (24 - j*8)) & 0xFF] << (24 - j*8);
            }
            temp = sub ^ ((uint32_t)rcon[i/Nk - 1] << 24);
        } else if (Nk > 6 && (i % Nk == 4)) {
            // AES-256额外处理
            uint32_t sub = 0;
            for (int j = 0; j < 4; j++) {
                sub |= (uint32_t)sbox[(temp >> (24 - j*8)) & 0xFF] << (24 - j*8);
            }
            temp = sub;
        }
        
        w[i] = w[i-Nk] ^ temp;
    }
}

// AES加密函数 (ECB模式)
void aes_encrypt(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *key) {
    uint32_t w[Nb * (Nr + 1)];
    key_expansion(key, w);
    
    for (size_t offset = 0; offset < length; offset += 16) {
        uint8_t state[4][4];
        
        // 初始化状态矩阵
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                state[j][i] = input[offset + i*4 + j];
            }
        }
        
        // 初始轮
        add_round_key(state, w, 0);
        
        // 主轮
        for (int round = 1; round < Nr; round++) {
            sub_bytes(state);
            shift_rows(state);
            mix_columns(state);
            add_round_key(state, w, round);
        }
        
        // 最终轮
        sub_bytes(state);
        shift_rows(state);
        add_round_key(state, w, Nr);
        
        // 写入输出
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                output[offset + i*4 + j] = state[j][i];
            }
        }
    }
}

// AES解密函数 (ECB模式)
void aes_decrypt(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *key) {
    uint32_t w[Nb * (Nr + 1)];
    key_expansion(key, w);
    
    for (size_t offset = 0; offset < length; offset += 16) {
        uint8_t state[4][4];
        
        // 初始化状态矩阵
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                state[j][i] = input[offset + i*4 + j];
            }
        }
        
        // 初始轮
        add_round_key(state, w, Nr);
        
        // 主轮
        for (int round = Nr-1; round > 0; round--) {
            inv_shift_rows(state);
            inv_sub_bytes(state);
            add_round_key(state, w, round);
            inv_mix_columns(state);
        }
        
        // 最终轮
        inv_shift_rows(state);
        inv_sub_bytes(state);
        add_round_key(state, w, 0);
        
        // 写入输出
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                output[offset + i*4 + j] = state[j][i];
            }
        }
    }
}

// PKCS#7填充
static int pkcs7_pad(const uint8_t *input, uint8_t *output, size_t input_len, 
                    size_t *output_len, size_t max_output_len) 
{
    const size_t block_size = 16;
    const size_t padding = block_size - (input_len % block_size);
    const size_t required_len = input_len + padding;
    
    // 1. 检查输出缓冲区是否足够
    if (required_len > max_output_len) {
        return -1; // 缓冲区不足，明确错误码
    }
    
    // 2. 复制原始数据
    memcpy(output, input, input_len);
    
    // 3. 添加填充
    for (size_t i = 0; i < padding; i++) {
        output[input_len + i] = (uint8_t)padding;
    }
    
    // 4. 设置输出长度
    *output_len = required_len;
    return 0; // 成功
}

// PKCS#7去除填充
static int pkcs7_unpad(const uint8_t *data, size_t *len) {
    if (*len == 0 || *len % 16 != 0) 
        return -1;
    
    // 安全获取填充值
    uint8_t padding = data[*len - 1];
    if (padding == 0 || padding > 16 || padding > *len) 
        return -1;
    
    // 安全验证填充字节
    for (size_t i = *len - padding; i < *len; i++) {
        if (data[i] != padding) 
            return -1;
    }
    
    *len -= padding;
    return 0;
}

// 带填充的加密函数
void aes_encrypt_padded(const uint8_t *input, uint8_t *output, size_t *len, 
                       const uint8_t *key) 
{
    // 1. 准备填充
    uint8_t padded_data[*len + 16]; // 栈分配临时缓冲区
    size_t padded_len = 0;
    
    // 2. 安全填充
    if (pkcs7_pad(input, padded_data, *len, &padded_len, sizeof(padded_data)) != 0) {
        *len = 0;
        return;
    }
    
    // 3. 加密填充后的数据
    aes_encrypt(padded_data, output, padded_len, key);
    *len = padded_len;
}

// 带填充的解密函数
void aes_decrypt_padded(const uint8_t *input, uint8_t *output, size_t *len, const uint8_t *key) {
    // 1. 先解密到临时缓冲区
    size_t temp_len = *len;
    uint8_t *temp_buf = malloc(temp_len);
    if (!temp_buf) {
        *len = 0;
        return;
    }
    
    aes_decrypt(input, temp_buf, temp_len, key);
    
    // 2. 从临时缓冲区移除填充
    int unpad_result = pkcs7_unpad(temp_buf, &temp_len);
    
    // 3. 复制有效数据到输出缓冲区
    if (unpad_result == 0 && temp_len > 0) {
        memcpy(output, temp_buf, temp_len);
        *len = temp_len;
    } else {
        *len = 0;  // 解密失败
    }
    
    free(temp_buf);
}

// 辅助函数：打印十六进制数据
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}


// 安全打印函数（避免乱码）
void safe_print(const char* message) {
    const char *p = message;
    while (*p) {
        if (*p == '\r' || *p == '\n') {
            putchar(*p);
        } else if (*p >= 32 && *p <= 126) {
            putchar(*p); // 可打印ASCII字符
        } else {
            putchar('.'); // 替换不可打印字符
        }
        p++;
    }
}
