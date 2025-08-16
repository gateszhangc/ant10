@echo off
title Bot Killer - Terminate All Bot Processes
color 0A

echo ===== Bot Killer Script =====
echo This script will terminate all bot processes and clean up bot files
echo.
echo Press any key to continue or close this window to cancel...
pause >nul

echo.
echo [1] Killing bot processes...

echo Killing temporary bot processes (temp_*.exe)...
tasklist | findstr /i "temp_" >nul 2>&1
if %errorlevel% == 0 (
    taskkill /f /im "temp_*.exe" >nul 2>&1
    echo - Killed temp processes
) else (
    echo - No temp processes found
)

echo Killing winlogonhlp.exe processes...
tasklist | findstr /i "winlogonhlp.exe" >nul 2>&1
if %errorlevel% == 0 (
    taskkill /f /im winlogonhlp.exe >nul 2>&1
    echo - Killed winlogonhlp.exe
) else (
    echo - No winlogonhlp.exe found
)

echo Killing bot.exe processes...
tasklist | findstr /i "bot.exe" >nul 2>&1
if %errorlevel% == 0 (
    taskkill /f /im bot.exe >nul 2>&1
    echo - Killed bot.exe
) else (
    echo - No bot.exe found
)

echo.
echo [2] Cleaning up bot files...

set BOT_PATH=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\System\Windows
echo Bot path: %BOT_PATH%

if exist "%BOT_PATH%" (
    echo Removing bot installation directory...
    
    if exist "%BOT_PATH%\winupdate.dat" (
        echo - Removing marker file...
        del /f /q "%BOT_PATH%\winupdate.dat" >nul 2>&1
    )
    
    if exist "%BOT_PATH%\winlogonhlp.exe" (
        echo - Removing bot executable...
        del /f /q "%BOT_PATH%\winlogonhlp.exe" >nul 2>&1
    )
    
    rmdir /s /q "%BOT_PATH%" >nul 2>&1
    echo - Directory removed
) else (
    echo - Bot installation directory not found
)

echo Cleaning temporary files...
del /f /q "%TEMP%\temp_*.exe" >nul 2>&1
echo - Temp files cleaned

echo.
echo [3] Cleaning registry entries...
echo Removing registry startup entries...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "winlogonhlp" /f >nul 2>&1
reg delete "HKLM\Software\Microsoft\Windows\CurrentVersion\Run" /v "winlogonhlp" /f >nul 2>&1
echo - Registry entries cleaned

echo.
echo [4] Cleaning scheduled tasks...
echo Removing scheduled tasks...
schtasks /delete /tn "winlogonhlp" /f >nul 2>&1
schtasks /delete /tn "Windows Update Helper" /f >nul 2>&1
echo - Scheduled tasks cleaned

echo.
echo [5] Verification...
echo Checking for remaining processes...
tasklist | findstr /i "temp_ winlogonhlp bot.exe" >nul 2>&1
if %errorlevel% == 0 (
    echo WARNING: Some bot processes may still be running!
    tasklist | findstr /i "temp_ winlogonhlp bot.exe"
) else (
    echo OK: No bot processes found
)

echo.
echo Checking for remaining files...
if exist "%BOT_PATH%\winupdate.dat" (
    echo WARNING: Marker file still exists
) else (
    echo OK: No marker file found
)

if exist "%BOT_PATH%\winlogonhlp.exe" (
    echo WARNING: Bot executable still exists
) else (
    echo OK: No bot executable found
)

echo.
echo ===== Cleanup Complete =====
echo.
echo If you see any WARNING messages above:
echo 1. Try running this script as Administrator
echo 2. Restart your computer and run again
echo 3. Check Task Manager for remaining processes
echo.
echo Press any key to exit...
pause >nul
