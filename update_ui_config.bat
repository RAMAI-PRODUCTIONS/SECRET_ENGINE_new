@echo off
REM Update UI config on device without rebuilding
echo Updating UI config on device...
echo.

REM Copy to assets folder
copy /Y ui_config.json android\app\src\main\assets\ui_config.json
echo ✓ Copied to assets folder

REM Push directly to device (if app is already installed)
echo.
echo Pushing to device...
adb push ui_config.json /sdcard/Android/data/com.secretengine.game/files/ui_config.json
if %errorlevel% neq 0 (
    echo Note: Direct push failed (app may not be installed yet)
    echo You'll need to rebuild and reinstall the app.
)

echo.
echo Done! Restart the app to see changes.
pause
