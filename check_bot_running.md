# ��μ��Bot�Ƿ�������

## Bot��������

����bot.c�Ĵ��������bot����ʱ��������������

### 1. ��������
- **������**: ������ʾΪ��ʱ�ļ������� `temp_xxxxxxxx.exe`��
- **����λ��**: ��Windows��ʱĿ¼��ִ��
- **�־û�**: �᳢�Ը�������ϵͳĿ¼�������־û�

### 2. �ļ�ϵͳ����
Bot��������λ�ô����ļ���

#### ��װ·��
```
%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winlogonhlp.exe
```

#### ����ļ�
```
%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winupdate.dat
```

### 3. ����
- **ͨ��**: ÿ25-30�볢������C2������
- **�˿�**: ����ʹ��HTTP/HTTPSЭ��
- **Ŀ��**: �������ӵ����õ�C2��������ַ

### 4. ϵͳ��Ϊ
- **Ȩ������**: ���Ի�ȡ����ԱȨ��
- **�ļ�����**: �����Ѱ�װ���ļ���ֹɾ��
- **������**: �����ƻ������ע�����ʵ�ֿ�������

## ��ⷽ��

### ����1: ������
��Windows����������в鿴��
1. ����������� (Ctrl+Shift+Esc)
2. �鿴"����"ѡ�
3. Ѱ�ҿ��ɵ���ʱ��������temp_xxxxxxxx.exe��
4. �鿴���̵�CPU���ڴ�ʹ�����

### ����2: ����ļ�ϵͳ
��Windows�м������·����

```cmd
# ��鰲װĿ¼
dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\"

# ������ļ�
dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winupdate.dat"

# ��鰲װ�Ŀ�ִ���ļ�
dir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows\winlogonhlp.exe"
```

### ����3: �����������
ʹ��netstat����鿴�������ӣ�

```cmd
# �鿴������������
netstat -an

# �鿴�ض����̵���������
netstat -ano | findstr :80
netstat -ano | findstr :443
```

### ����4: ���ƻ�����
```cmd
# �鿴�ƻ�����
schtasks /query /fo LIST /v | findstr "winlogonhlp"
```

### ����5: ���ע���
```cmd
# ���������
reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run"
reg query "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run"
```

## PowerShell���ű�

��������PowerShell�ű����Զ���⣺

```powershell
# ���Bot����״̬
Write-Host "=== Bot Detection Script ===" -ForegroundColor Yellow

# 1. ������
Write-Host "`n[1] Checking processes..." -ForegroundColor Cyan
$tempProcesses = Get-Process | Where-Object { $_.ProcessName -like "temp_*" }
if ($tempProcesses) {
    Write-Host "Found suspicious temp processes:" -ForegroundColor Red
    $tempProcesses | Format-Table ProcessName, Id, CPU, WorkingSet
} else {
    Write-Host "No suspicious temp processes found" -ForegroundColor Green
}

# 2. ����ļ�
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

# 3. �����������
Write-Host "`n[3] Checking network connections..." -ForegroundColor Cyan
$connections = netstat -ano | Select-String ":80|:443|:8080|:8443"
if ($connections) {
    Write-Host "Active network connections:" -ForegroundColor Yellow
    $connections
} else {
    Write-Host "No suspicious network connections found" -ForegroundColor Green
}

# 4. ���ƻ�����
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

## �򵥼������

���������ټ�⣬������Windows������ʾ�������У�

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

## Ԥ�ڽ��

���bot�ɹ����У���Ӧ���ܿ�����
1. ? ����ʱĿ¼���л�Ծ��temp_xxxxxxxx.exe����
2. ? �����˱���ļ� winupdate.dat
3. ? �����˿�ִ���ļ� winlogonhlp.exe
4. ? �ж��ڵ��������ӳ���
5. ? �����мƻ������ע�����

���ֻ����ExitLag���е�û������������˵��bot����û�гɹ������򱻰�ȫ�����ֹ�ˡ�
