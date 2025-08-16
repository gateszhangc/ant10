@echo off
title Force Bot Killer - Nuclear Option
color 0C

echo ===== FORCE BOT KILLER - NUCLEAR OPTION =====
echo This script will forcefully terminate ALL bot processes
echo and clean up ALL related files using maximum force.
echo.
echo WARNING: This will use administrative privileges!
echo.
echo Press any key to continue or close this window to cancel...
pause >nul

echo.
echo [STEP 1] Requesting Administrator Privileges...
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo This script requires administrator privileges!
    echo Please run as administrator or the script will try to elevate...
    
    REM Try to self-elevate
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo Running with administrator privileges...

echo.
echo [STEP 2] Nuclear Process Termination...

echo Killing ALL temp processes...
wmic process where "name like 'temp_%%'" delete >nul 2>&1

echo Killing winlogonhlp.exe with extreme prejudice...
taskkill /f /im winlogonhlp.exe /t >nul 2>&1
wmic process where "name='winlogonhlp.exe'" delete >nul 2>&1

echo Killing bot.exe processes...
taskkill /f /im bot.exe /t >nul 2>&1
wmic process where "name='bot.exe'" delete >nul 2>&1

echo Killing any process with 'temp_' in command line...
for /f "tokens=2" %%i in ('wmic process where "commandline like '%%temp_%%'" get processid /format:csv ^| findstr /r "[0-9]"') do (
    taskkill /f /pid %%i >nul 2>&1
)

echo.
echo [STEP 3] File System Nuclear Cleanup...

set BOT_PATH=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows

echo Unlocking and deleting bot files...
if exist "%BOT_PATH%" (
    echo Taking ownership of bot directory...
    takeown /f "%BOT_PATH%" /r /d y >nul 2>&1
    icacls "%BOT_PATH%" /grant administrators:F /t >nul 2>&1
    
    echo Removing file attributes...
    attrib -r -h -s "%BOT_PATH%\*.*" /s >nul 2>&1
    
    echo Force deleting marker file...
    if exist "%BOT_PATH%\winupdate.dat" (
        del /f /q /a "%BOT_PATH%\winupdate.dat" >nul 2>&1
    )
    
    echo Force deleting bot executable...
    if exist "%BOT_PATH%\winlogonhlp.exe" (
        del /f /q /a "%BOT_PATH%\winlogonhlp.exe" >nul 2>&1
    )
    
    echo Removing directory structure...
    rmdir /s /q "%BOT_PATH%" >nul 2>&1
)

echo Cleaning ALL temporary files...
del /f /q /s "%TEMP%\temp_*.exe" >nul 2>&1
del /f /q /s "%TMP%\temp_*.exe" >nul 2>&1
del /f /q /s "C:\Windows\Temp\temp_*.exe" >nul 2>&1

echo.
echo [STEP 4] Registry Nuclear Cleanup...
echo Removing ALL possible registry entries...

reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "winlogonhlp" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "winlogonhlp" /f >nul 2>&1
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "Windows Update Helper" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "Windows Update Helper" /f >nul 2>&1
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "System Update" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "System Update" /f >nul 2>&1

REM Clean RunOnce entries
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\RunOnce" /v "winlogonhlp" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\RunOnce" /v "winlogonhlp" /f >nul 2>&1

echo.
echo [STEP 5] Scheduled Tasks Nuclear Cleanup...
echo Removing ALL possible scheduled tasks...

schtasks /delete /tn "winlogonhlp" /f >nul 2>&1
schtasks /delete /tn "Windows Update Helper" /f >nul 2>&1
schtasks /delete /tn "System Update" /f >nul 2>&1
schtasks /delete /tn "Microsoft\Windows\winlogonhlp" /f >nul 2>&1

echo.
echo [STEP 6] Service Cleanup...
echo Stopping and removing any bot services...

sc stop "winlogonhlp" >nul 2>&1
sc delete "winlogonhlp" >nul 2>&1
sc stop "Windows Update Helper" >nul 2>&1
sc delete "Windows Update Helper" >nul 2>&1

echo.
echo [STEP 7] Final Verification...

echo Checking for remaining processes...
set FOUND_PROCS=0
for /f %%i in ('tasklist /fo csv ^| findstr /i "temp_\|winlogonhlp\|bot.exe" ^| find /c /v ""') do set FOUND_PROCS=%%i

if %FOUND_PROCS% gtr 0 (
    echo WARNING: %FOUND_PROCS% bot processes still running!
    echo Listing remaining processes:
    tasklist | findstr /i "temp_ winlogonhlp bot.exe"
    echo.
    echo Try these manual steps:
    echo 1. Open Task Manager (Ctrl+Shift+Esc)
    echo 2. Go to Details tab
    echo 3. Manually end any temp_* or winlogonhlp.exe processes
    echo 4. Restart your computer
) else (
    echo SUCCESS: No bot processes found!
)

echo.
echo Checking for remaining files...
if exist "%BOT_PATH%\winupdate.dat" (
    echo WARNING: Marker file still exists: %BOT_PATH%\winupdate.dat
    echo Try manually deleting this file
) else (
    echo SUCCESS: No marker file found
)

if exist "%BOT_PATH%\winlogonhlp.exe" (
    echo WARNING: Bot executable still exists: %BOT_PATH%\winlogonhlp.exe
    echo Try manually deleting this file
) else (
    echo SUCCESS: No bot executable found
)

echo.
echo ===== NUCLEAR CLEANUP COMPLETE =====
echo.
echo Summary of actions taken:
echo - Terminated all bot processes (including child processes)
echo - Removed all bot files and directories
echo - Cleaned registry startup entries
echo - Removed scheduled tasks
echo - Stopped and removed services
echo - Cleaned temporary files
echo.
echo If you still see WARNING messages:
echo 1. Restart your computer
echo 2. Run this script again after restart
echo 3. Check for rootkit/malware with antivirus
echo 4. Consider system restore if problems persist
echo.
echo Press any key to exit...
pause >nul
