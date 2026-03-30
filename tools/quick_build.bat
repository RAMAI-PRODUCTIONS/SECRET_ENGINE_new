@echo off
cd /d "%~dp0.."
echo ========================================
echo   SecretEngine - Quick Build Script
echo ========================================
echo.

echo [1/3] Building APK...
cd android
call gradlew.bat assembleDebug
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)
cd ..

echo.
echo [2/3] Installing APK...
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Installation failed! Is device connected?
    echo Run 'adb devices' to check.
    pause
    exit /b 1
)

echo.
echo [3/3] Starting logcat monitor...
echo Press Ctrl+C to stop monitoring
echo.
adb logcat | findstr SecretEngine

pause
