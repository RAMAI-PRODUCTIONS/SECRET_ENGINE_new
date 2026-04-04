@echo off
REM Validate docs structure - only README.md should be in root docs/

echo Validating docs/ structure...

set "invalid_count=0"
set "invalid_files="

for %%f in (docs\*.md docs\*.html) do (
    if exist "%%f" (
        set "filename=%%~nxf"
        if not "!filename!"=="README.md" (
            set /a invalid_count+=1
            echo   - %%f
            set "invalid_files=!invalid_files! %%f"
        )
    )
)

if %invalid_count%==0 (
    echo [32m✓ Docs structure is valid - only README.md in root[0m
    exit /b 0
) else (
    echo [31m✗ Invalid docs structure detected![0m
    echo.
    echo The following files should be moved to subdirectories:
    echo %invalid_files%
    echo.
    echo Available subdirectories:
    echo   - docs/architecture/    ^(Engine architecture and design^)
    echo   - docs/features/        ^(Feature documentation^)
    echo   - docs/guides/          ^(How-to guides^)
    echo   - docs/reference/       ^(API references^)
    echo   - docs/implementation/  ^(Implementation details^)
    echo   - docs/fixes/           ^(Bug fixes^)
    echo   - docs/research/        ^(Research notes^)
    echo   - docs/status/          ^(Status reports^)
    echo   - docs/planning/        ^(Planning docs^)
    echo   - docs/foundation/      ^(Design principles^)
    exit /b 1
)
