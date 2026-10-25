@echo off
cd /d "%~dp0\..\backend"
echo ==> Building C backend...
mingw32-make all
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Backend compilation failed.
    pause
    exit /b %ERRORLEVEL%
)
echo ==> Starting Intelligent OS Analytics Backend on port 8080...
bin\os_platform_backend.exe --port 8080
