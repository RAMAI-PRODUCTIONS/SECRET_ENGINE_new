@echo off
echo ========================================
echo Launch App and Monitor Logs
echo ========================================
echo.

REM Launch the app
echo Launching SecretEngine...
adb shell am start -n com.secretengine.game/.MainActivity
echo.

REM Wait a moment for app to start
timeout /t 2 /nobreak > nul

REM Clear old logs and start monitoring
echo Starting log monitor...
echo Press Ctrl+C to stop
echo.
adb logcat -c
adb logcat | findstr /I "SecretEngine AndroidInput FPSUI native crash FATAL"
