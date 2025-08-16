#include "bot.h"

// ����ΨһBot ID
void generate_bot_id() {
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        BYTE raw_id[BOT_ID_SIZE];
        if (CryptGenRandom(hProv, BOT_ID_SIZE, raw_id)) {
            for (int i = 0; i < BOT_ID_SIZE; i++) {
                // ��������ɶ���ID (ʮ�����Ƹ�ʽ)
                sprintf_s(bot_id + i*2, BOT_ID_SIZE - i*2, "%02X", raw_id[i]);
            }
            bot_id[BOT_ID_SIZE - 1] = '\0';
        } else {
            strcpy_s(bot_id, BOT_ID_SIZE, "DEFAULTBOTID"); // ���˷���
        }
        CryptReleaseContext(hProv, 0);
    } else {
        strcpy_s(bot_id, BOT_ID_SIZE, "DEFAULTBOTID");
    }
}