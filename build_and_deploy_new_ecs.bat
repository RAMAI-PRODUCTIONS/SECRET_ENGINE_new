@echo off
echo ========================================
echo Building SecretEngine with New ECS System
echo ========================================

cd android

echo.
echo [1/4] Cleaning previous build...
call gradlew clean

echo.
echo [2/4] Building APK with new ECS system...
call gradlew assembleDebug

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Build failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Installing APK to device...
adb install -r app\build\outputs\apk\debug\app-debug.apk

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Installation failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Launching app...
adb shell am start -n com.secretengine.game/.MainActivity

echo.
echo ========================================
echo ✅ Build and deployment complete!
echo ========================================
echo.
echo The app should now be running on your device with:
echo - New ECS system active
echo - chunk_0_full.json loaded
echo - Scene with EntityObjects and Components
echo - Fast component access for rendering
echo.
echo Check logcat for detailed logs:
echo   adb logcat -s SecretEngine:* V73LevelSystemPlugin:* SceneLoader:*
echo.
pause
