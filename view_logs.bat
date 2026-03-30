@echo off
echo ========================================
echo SecretEngine Live Logs
echo ========================================
echo Press Ctrl+C to stop
echo.
adb logcat -c
adb logcat | findstr /I "SecretEngine AndroidInput FPSUI Vulkan"
