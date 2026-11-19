@echo off
cd /d "%~dp0\..\backend"
echo ===============================================================================
echo   Intelligent Operating System Analytics - Interactive Terminal Dashboard
echo ===============================================================================
echo.
echo [*] Checking build artifacts...
if not exist "bin\os_platform_cli.exe" (
    echo [*] CLI binary not found. Building with mingw32-make...
    mingw32-make bin/os_platform_cli.exe
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Compilation failed. Please ensure MinGW-w64 gcc is installed and on PATH.
        pause
        exit /b %ERRORLEVEL%
    )
)

echo [*] Launching Interactive Terminal UI...
echo.
bin\os_platform_cli.exe
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [*] CLI exited with code %ERRORLEVEL%.
    pause
)
