#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== Windows 8-11����Binder ===\n");
    
    if (argc < 3) {
        printf("�÷�: %s <stubloader.exe> <payload.exe> [����ļ�]\n", argv[0]);
        return 1;
    }
    
    char* stub_path = argv[1];
    char* payload_path = argv[2];
    char* output_path = (argc > 3) ? argv[3] : "output_win8to11.exe";
    
    printf("Stubloader: %s\n", stub_path);
    printf("Payload: %s\n", payload_path);
    printf("���: %s\n", output_path);
    
    // ��ȡstubloader
    FILE* stub_file = fopen(stub_path, "rb");
    if (!stub_file) {
        printf("����: �޷���stubloader�ļ�\n");
        return 1;
    }
    
    fseek(stub_file, 0, SEEK_END);
    long stub_size = ftell(stub_file);
    fseek(stub_file, 0, SEEK_SET);
    
    char* stub_data = malloc(stub_size);
    fread(stub_data, 1, stub_size, stub_file);
    fclose(stub_file);
    
    // ��ȡpayload
    FILE* payload_file = fopen(payload_path, "rb");
    if (!payload_file) {
        printf("����: �޷���payload�ļ�\n");
        free(stub_data);
        return 1;
    }
    
    fseek(payload_file, 0, SEEK_END);
    long payload_size = ftell(payload_file);
    fseek(payload_file, 0, SEEK_SET);
    
    char* payload_data = malloc(payload_size);
    fread(payload_data, 1, payload_size, payload_file);
    fclose(payload_file);
    
    // ��������ļ�
    FILE* output_file = fopen(output_path, "wb");
    if (!output_file) {
        printf("����: �޷���������ļ�\n");
        free(stub_data);
        free(payload_data);
        return 1;
    }
    
    // д��stubloader
    fwrite(stub_data, 1, stub_size, output_file);
    
    // д����
    const char marker[] = "WIN8TO11_PAYLOAD_MARKER";
    fwrite(marker, 1, sizeof(marker), output_file);
    
    // д��payload��С
    fwrite(&payload_size, sizeof(payload_size), 1, output_file);
    
    // д��payload
    fwrite(payload_data, 1, payload_size, output_file);
    
    fclose(output_file);
    
    printf("�����!\n");
    printf("����ļ���С: %ld �ֽ�\n", stub_size + payload_size + sizeof(marker) + sizeof(payload_size));
    
    free(stub_data);
    free(payload_data);
    
    return 0;
}
