@echo off
echo ========================================
echo Android Build and Deploy Script
echo ========================================
echo.

REM Step 1: Copy UI config to assets
echo [1/4] Copying UI config to assets...
copy /Y ui_config.json android\app\src\main\assets\ui_config.json
if %errorlevel% neq 0 (
    echo ERROR: Failed to copy ui_config.json
    pause
    exit /b 1
)
echo ✓ UI config copied
echo.

REM Step 2: Build APK
echo [2/4] Building APK (Debug)...
cd android
call gradlew.bat assembleDebug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    cd ..
    pause
    exit /b 1
)
cd ..
echo ✓ Build successful
echo.

REM Step 3: Check for connected device
echo [3/4] Checking for connected device...
adb devices
echo.
echo Make sure your device is connected and USB debugging is enabled.
echo Press any key to continue with installation...
pause > nul

REM Step 4: Install APK
echo [4/4] Installing APK on device...
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
if %errorlevel% neq 0 (
    echo ERROR: Installation failed
    echo.
    echo Troubleshooting:
    echo - Make sure USB debugging is enabled
    echo - Check if device is authorized (check device screen)
    echo - Try: adb kill-server ^&^& adb start-server
    pause
    exit /b 1
)
echo.
echo ========================================
echo ✓ Deployment Complete!
echo ========================================
echo.
echo The app should now be installed on your device.
echo Look for the app icon and launch it.
echo.
pause
