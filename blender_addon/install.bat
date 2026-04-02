@echo off
echo Secret Engine Level Editor - Installation Script
echo.

set BLENDER_VERSION=4.0
set ADDON_NAME=secret_engine_level_editor

echo Detecting Blender addons directory...
set ADDONS_DIR=%APPDATA%\Blender Foundation\Blender\%BLENDER_VERSION%\scripts\addons

if not exist "%ADDONS_DIR%" (
    echo Blender addons directory not found at: %ADDONS_DIR%
    echo.
    echo Please enter your Blender version (e.g., 3.6, 4.0, 4.1):
    set /p BLENDER_VERSION="Version: "
    set ADDONS_DIR=%APPDATA%\Blender Foundation\Blender\!BLENDER_VERSION!\scripts\addons
)

if not exist "%ADDONS_DIR%" (
    echo Creating addons directory: %ADDONS_DIR%
    mkdir "%ADDONS_DIR%"
)

echo.
echo Installing addon to: %ADDONS_DIR%\%ADDON_NAME%
echo.

if exist "%ADDONS_DIR%\%ADDON_NAME%" (
    echo Removing old version...
    rmdir /s /q "%ADDONS_DIR%\%ADDON_NAME%"
)

echo Copying addon files...
xcopy /E /I /Y "%~dp0%ADDON_NAME%" "%ADDONS_DIR%\%ADDON_NAME%"

echo.
echo Installation complete!
echo.
echo Next steps:
echo 1. Open Blender
echo 2. Go to Edit ^> Preferences ^> Add-ons
echo 3. Search for "Secret Engine Level Editor"
echo 4. Enable the addon by checking the checkbox
echo.
pause
