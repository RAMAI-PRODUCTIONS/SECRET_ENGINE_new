@echo off
REM Quick deploy - assumes build is already done, just reinstall
echo Quick Deploy - Installing APK...
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
if %errorlevel% neq 0 (
    echo Installation failed. Run deploy_android.bat for full build.
    pause
    exit /b 1
)
echo ✓ Installed successfully!
pause
