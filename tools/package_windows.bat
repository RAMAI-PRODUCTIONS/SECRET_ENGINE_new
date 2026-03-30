@echo off
cd /d "%~dp0.."
echo Packaging SecretEngine for Windows...
if not exist "dist" mkdir dist
if not exist "dist\bin" mkdir dist\bin
if not exist "dist\plugins" mkdir dist\plugins
if not exist "dist\assets" mkdir dist\assets

echo Copying binaries...
xcopy /y "build\bin\Debug\*" "dist\bin\"
xcopy /y "build\bin\Release\*" "dist\bin\"

echo Copying plugins...
xcopy /s /y "build\tests\Debug\plugins\*" "dist\plugins\"

echo Copying assets...
xcopy /s /y "Assets\*" "dist\assets\"

echo Done! Package available in dist/ folder.
