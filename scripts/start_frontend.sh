#!/usr/bin/env bash
# Start Frontend on Linux/Ubuntu
set -e

cd "$(dirname "$0")/../frontend"
if [ ! -d "node_modules" ]; then
    echo "==> Installing frontend dependencies..."
    npm install
fi

echo "==> Starting Vite development server on port 5173..."
npm run dev -- --host 0.0.0.0 --port 5173
