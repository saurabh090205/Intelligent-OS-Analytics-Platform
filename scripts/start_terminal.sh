#!/usr/bin/env bash
# Start Intelligent Operating System Analytics Terminal UI on Linux / POSIX
set -e

cd "$(dirname "$0")/../backend"
echo "==============================================================================="
echo "  Intelligent Operating System Analytics - Interactive Terminal Dashboard"
echo "==============================================================================="
echo ""

if [ ! -f "bin/os_platform_cli" ]; then
    echo "[*] CLI binary not found. Building with make..."
    make bin/os_platform_cli
fi

echo "[*] Launching Interactive Terminal UI..."
echo ""
./bin/os_platform_cli
