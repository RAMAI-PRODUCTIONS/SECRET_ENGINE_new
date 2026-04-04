@echo off
REM Build script for Cyberpunk City Demo
REM Automates shader compilation and Android build

echo ========================================
echo   CYBERPUNK CITY DEMO - BUILD SCRIPT
echo ========================================
echo.

REM Step 1: Compile shaders
echo [1/3] Compiling shaders...
call compile_shaders.bat
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Shader compilation failed!
    exit /b 1
)
echo.

REM Step 2: Build Android APK
echo [2/3] Building Android APK...
cd android
call gradlew assembleDebug
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Android build failed!
    cd ..
    exit /b 1
)
cd ..
echo.

REM Step 3: Install on device (if connected)
echo [3/3] Installing on device...
adb devices | find "device" >nul
if %ERRORLEVEL% EQU 0 (
    adb install -r android/app/build/outputs/apk/debug/app-debug.apk
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo ========================================
        echo   BUILD COMPLETE! APK INSTALLED!
        echo ========================================
        echo.
        echo Launch "SecretEngine" on your device
        echo.
    ) else (
        echo WARNING: Installation failed. APK built but not installed.
    )
) else (
    echo WARNING: No Android device connected.
    echo APK built at: android/app/build/outputs/apk/debug/app-debug.apk
)

echo.
echo Build process complete!
pause
