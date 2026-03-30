@echo off
echo ========================================
echo SecretEngine App Status Check
echo ========================================
echo.

echo [1] Checking if app is installed...
adb shell pm list packages | findstr secretengine
if %errorlevel% equ 0 (
    echo ✓ App is installed
) else (
    echo ✗ App not found
)
echo.

echo [2] Checking if app is running...
adb shell "ps | grep secretengine"
if %errorlevel% equ 0 (
    echo ✓ App is running
) else (
    echo ✗ App not running
)
echo.

echo [3] Checking recent logs...
adb logcat -d -t 50 | findstr /I "secretengine crash fatal error"
echo.

echo [4] App Info:
adb shell dumpsys package com.secretengine.game | findstr "versionName versionCode"
echo.

echo ========================================
echo Commands:
echo - Launch: adb shell am start -n com.secretengine.game/.MainActivity
echo - Stop: adb shell am force-stop com.secretengine.game
echo - Logs: view_logs.bat
echo ========================================
pause
