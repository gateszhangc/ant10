#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== Windows 8-11兼容Binder ===\n");
    
    if (argc < 3) {
        printf("用法: %s <stubloader.exe> <payload.exe> [输出文件]\n", argv[0]);
        return 1;
    }
    
    char* stub_path = argv[1];
    char* payload_path = argv[2];
    char* output_path = (argc > 3) ? argv[3] : "output_win8to11.exe";
    
    printf("Stubloader: %s\n", stub_path);
    printf("Payload: %s\n", payload_path);
    printf("输出: %s\n", output_path);
    
    // 读取stubloader
    FILE* stub_file = fopen(stub_path, "rb");
    if (!stub_file) {
        printf("错误: 无法打开stubloader文件\n");
        return 1;
    }
    
    fseek(stub_file, 0, SEEK_END);
    long stub_size = ftell(stub_file);
    fseek(stub_file, 0, SEEK_SET);
    
    char* stub_data = malloc(stub_size);
    fread(stub_data, 1, stub_size, stub_file);
    fclose(stub_file);
    
    // 读取payload
    FILE* payload_file = fopen(payload_path, "rb");
    if (!payload_file) {
        printf("错误: 无法打开payload文件\n");
        free(stub_data);
        return 1;
    }
    
    fseek(payload_file, 0, SEEK_END);
    long payload_size = ftell(payload_file);
    fseek(payload_file, 0, SEEK_SET);
    
    char* payload_data = malloc(payload_size);
    fread(payload_data, 1, payload_size, payload_file);
    fclose(payload_file);
    
    // 创建输出文件
    FILE* output_file = fopen(output_path, "wb");
    if (!output_file) {
        printf("错误: 无法创建输出文件\n");
        free(stub_data);
        free(payload_data);
        return 1;
    }
    
    // 写入stubloader
    fwrite(stub_data, 1, stub_size, output_file);
    
    // 写入标记
    const char marker[] = "WIN8TO11_PAYLOAD_MARKER";
    fwrite(marker, 1, sizeof(marker), output_file);
    
    // 写入payload大小
    fwrite(&payload_size, sizeof(payload_size), 1, output_file);
    
    // 写入payload
    fwrite(payload_data, 1, payload_size, output_file);
    
    fclose(output_file);
    
    printf("绑定完成!\n");
    printf("输出文件大小: %ld 字节\n", stub_size + payload_size + sizeof(marker) + sizeof(payload_size));
    
    free(stub_data);
    free(payload_data);
    
    return 0;
}
