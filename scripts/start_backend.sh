#!/usr/bin/env bash
# Start C Backend on Linux/Ubuntu
set -e

cd "$(dirname "$0")/../backend"
echo "==> Building C backend..."
make all

echo "==> Starting Intelligent OS Analytics Backend on port 8080..."
./bin/os_platform_backend --port 8080
