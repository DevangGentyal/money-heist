#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

echo "🧹 Cleaning old build outputs..."
rm -f heist_gui

echo ""
echo "🔁 Rebuilding GUI from scratch..."
bash build_gui.sh

echo ""
echo "✅ GUI-only rebuild complete"
echo "Run GUI: ./heist_gui"