@echo off
REM C++26 Device Test Script for Windows
REM Tests all C++26 features on Android device

echo ========================================
echo C++26 DEVICE TEST - SECRET_ENGINE
echo ========================================
echo.

REM Check ADB
echo [1/8] Checking ADB connection...
adb devices
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: ADB not found or no device connected
    exit /b 1
)
echo OK
echo.

REM Build Debug APK
echo [2/8] Building debug APK...
cd android
call gradlew assembleDebug
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)
cd ..
echo OK
echo.

REM Install APK
echo [3/8] Installing APK on device...
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Installation failed
    exit /b 1
)
echo OK
echo.

REM Clear logcat
echo [4/8] Clearing logcat...
adb logcat -c
echo OK
echo.

REM Start logcat in background
echo [5/8] Starting logcat monitoring...
start "Logcat Monitor" cmd /c "adb logcat | findstr /I \"SecretEngine sanitizer ASAN UBSAN CPP26 span inplace_vector\" > cpp26_device_test_log.txt"
timeout /t 2 /nobreak >nul
echo OK
echo.

REM Launch app
echo [6/8] Launching app...
adb shell am start -n com.secretengine.game/.MainActivity
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to launch app
    exit /b 1
)
echo OK
echo.

REM Wait for app to initialize
echo [7/8] Waiting for app initialization (10 seconds)...
timeout /t 10 /nobreak
echo OK
echo.

REM Collect device info
echo [8/8] Collecting device info...
echo.
echo === DEVICE INFO ===
adb shell getprop ro.product.model
adb shell getprop ro.build.version.release
echo.

REM Show recent logs
echo === RECENT LOGS (Last 50 lines) ===
adb logcat -d -t 50 | findstr /I "SecretEngine sanitizer ASAN UBSAN CPP26"
echo.

echo ========================================
echo TEST COMPLETE
echo ========================================
echo.
echo Full log saved to: cpp26_device_test_log.txt
echo.
echo NEXT STEPS:
echo 1. Check cpp26_device_test_log.txt for detailed logs
echo 2. Verify sanitizers initialized
echo 3. Check for any crashes or errors
echo 4. Monitor frame rate in app
echo.
pause
