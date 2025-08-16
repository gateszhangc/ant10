# 如何检查Bot是否在运行

## Bot运行特征

根据bot.c的代码分析，bot运行时会有以下特征：

### 1. 进程特征
- **进程名**: 可能显示为临时文件名（如 `temp_xxxxxxxx.exe`）
- **运行位置**: 在Windows临时目录中执行
- **持久化**: 会尝试复制自身到系统目录并创建持久化

### 2. 文件系统特征
Bot会在以下位置创建文件：

#### 安装路径
```
%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winlogonhlp.exe
```

#### 标记文件
```
%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winupdate.dat
```

### 3. 网络活动
- **通信**: 每25-30秒尝试连接C2服务器
- **端口**: 可能使用HTTP/HTTPS协议
- **目标**: 尝试连接到配置的C2服务器地址

### 4. 系统行为
- **权限提升**: 尝试获取管理员权限
- **文件锁定**: 锁定已安装的文件防止删除
- **自启动**: 创建计划任务或注册表项实现开机自启

## 检测方法

### 方法1: 检查进程
在Windows任务管理器中查看：
1. 打开任务管理器 (Ctrl+Shift+Esc)
2. 查看"进程"选项卡
3. 寻找可疑的临时进程名（temp_xxxxxxxx.exe）
4. 查看进程的CPU和内存使用情况

### 方法2: 检查文件系统
在Windows中检查以下路径：

```cmd
# 检查安装目录
dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\"

# 检查标记文件
dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winupdate.dat"

# 检查安装的可执行文件
dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winlogonhlp.exe"
```

### 方法3: 检查网络连接
使用netstat命令查看网络连接：

```cmd
# 查看所有网络连接
netstat -an

# 查看特定进程的网络连接
netstat -ano | findstr :80
netstat -ano | findstr :443
```

### 方法4: 检查计划任务
```cmd
# 查看计划任务
schtasks /query /fo LIST /v | findstr "winlogonhlp"
```

### 方法5: 检查注册表
```cmd
# 检查启动项
reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run"
reg query "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run"
```

## PowerShell检测脚本

创建以下PowerShell脚本来自动检测：

```powershell
# 检测Bot运行状态
Write-Host "=== Bot Detection Script ===" -ForegroundColor Yellow

# 1. 检查进程
Write-Host "`n[1] Checking processes..." -ForegroundColor Cyan
$tempProcesses = Get-Process | Where-Object { $_.ProcessName -like "temp_*" }
if ($tempProcesses) {
    Write-Host "Found suspicious temp processes:" -ForegroundColor Red
    $tempProcesses | Format-Table ProcessName, Id, CPU, WorkingSet
} else {
    Write-Host "No suspicious temp processes found" -ForegroundColor Green
}

# 2. 检查文件
Write-Host "`n[2] Checking files..." -ForegroundColor Cyan
$botPath = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows"
$markerFile = "$botPath\winupdate.dat"
$botExe = "$botPath\winlogonhlp.exe"

if (Test-Path $markerFile) {
    Write-Host "Bot marker file found: $markerFile" -ForegroundColor Red
    Get-Item $markerFile | Format-List
} else {
    Write-Host "Bot marker file not found" -ForegroundColor Green
}

if (Test-Path $botExe) {
    Write-Host "Bot executable found: $botExe" -ForegroundColor Red
    Get-Item $botExe | Format-List
} else {
    Write-Host "Bot executable not found" -ForegroundColor Green
}

# 3. 检查网络连接
Write-Host "`n[3] Checking network connections..." -ForegroundColor Cyan
$connections = netstat -ano | Select-String ":80|:443|:8080|:8443"
if ($connections) {
    Write-Host "Active network connections:" -ForegroundColor Yellow
    $connections
} else {
    Write-Host "No suspicious network connections found" -ForegroundColor Green
}

# 4. 检查计划任务
Write-Host "`n[4] Checking scheduled tasks..." -ForegroundColor Cyan
$tasks = schtasks /query /fo CSV | ConvertFrom-Csv | Where-Object { $_.TaskName -like "*winlogon*" }
if ($tasks) {
    Write-Host "Found suspicious scheduled tasks:" -ForegroundColor Red
    $tasks | Format-Table
} else {
    Write-Host "No suspicious scheduled tasks found" -ForegroundColor Green
}

Write-Host "`n=== Detection Complete ===" -ForegroundColor Yellow
```

## 简单检测命令

如果你想快速检测，可以在Windows命令提示符中运行：

```cmd
@echo off
echo === Quick Bot Detection ===
echo.

echo [1] Checking for bot files...
if exist "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winupdate.dat" (
    echo [FOUND] Bot marker file detected!
    dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winupdate.dat"
) else (
    echo [OK] No bot marker file found
)

if exist "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winlogonhlp.exe" (
    echo [FOUND] Bot executable detected!
    dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winlogonhlp.exe"
) else (
    echo [OK] No bot executable found
)

echo.
echo [2] Checking for temp processes...
tasklist | findstr /i "temp_"

echo.
echo [3] Checking network connections...
netstat -ano | findstr ":80\|:443\|:8080"

echo.
echo === Detection Complete ===
pause
```

## 预期结果

如果bot成功运行，你应该能看到：
1. ? 在临时目录中有活跃的temp_xxxxxxxx.exe进程
2. ? 创建了标记文件 winupdate.dat
3. ? 复制了可执行文件 winlogonhlp.exe
4. ? 有定期的网络连接尝试
5. ? 可能有计划任务或注册表项

如果只看到ExitLag运行但没有上述特征，说明bot可能没有成功启动或被安全软件阻止了。
