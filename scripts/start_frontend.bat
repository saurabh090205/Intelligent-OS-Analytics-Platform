@echo off
cd /d "%~dp0\..\frontend"
if not exist "node_modules\" (
    echo ==> Installing frontend dependencies...
    call npm install
)
echo ==> Starting Vite development server on port 5173...
call npm run dev -- --host 127.0.0.1 --port 5173
