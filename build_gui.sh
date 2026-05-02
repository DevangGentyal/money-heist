#!/bin/bash
set -euo pipefail

# Money Heist 3D GUI - Build Script

echo "🎮 Money Heist 3D - GUI Edition Build Script"
echo "==========================================="

# Check if raylib is installed
if ! pkg-config --exists raylib; then
    echo "⚠️  Raylib not found. Installing..."
    
    # Check OS and install accordingly
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        echo "Detected macOS. Installing via Homebrew..."
        if ! command -v brew &> /dev/null; then
            echo "❌ Homebrew not found. Please install Homebrew first:"
            echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
            exit 1
        fi
        brew install raylib
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        echo "Detected Linux. Installing via apt..."
        sudo apt-get update
        sudo apt-get install -y libraylib-dev
    else
        echo "❌ Unsupported OS. Please install Raylib manually:"
        echo "   https://github.com/raysan5/raylib/wiki/Working-on-different-platforms"
        exit 1
    fi
fi

echo "✓ Raylib installed"

# Build
cd "$(dirname "$0")"

build_tag=$(date '+%Y-%m-%d %H:%M:%S %Z')

echo "🧹 Removing old GUI binary..."
rm -f heist_gui heist_game_ai debug_window

echo ""
echo "🔨 Compiling GUI version..."
clang++ -std=c++17 -Wall -Wextra \
    "-DHEIST_BUILD_TAG=\"$build_tag\"" \
    "-DHEIST_BUILD_FLAVOR=\"GUI\"" \
    "-DHEIST_GUI_BUILD_TAG=\"$build_tag\"" \
    src/gui_main.cpp \
    src/core/GameEngineGUI.cpp \
    src/grid/Grid3D.cpp \
    src/agents/Agent.cpp \
    src/agents/RobberAI.cpp \
    src/agents/PoliceAI.cpp \
    src/planning/*.cpp \
    src/ai/AStar3D.cpp \
    src/ai/HeuristicEngine.cpp \
    src/ai/PredictionEngine.cpp \
    src/rules/RuleEngine.cpp \
    src/rendering/RaylibRenderer.cpp \
    $(pkg-config --cflags --libs raylib) \
    -o heist_gui

echo "🔨 Compiling debug window..."
clang++ -std=c++17 -Wall -Wextra \
    "-DHEIST_BUILD_TAG=\"$build_tag\"" \
    "-DHEIST_BUILD_FLAVOR=\"DEBUG\"" \
    src/debug/debug_main.cpp \
    src/debug/DebugWindow.cpp \
    src/grid/Grid3D.cpp \
    src/ai/AStar3D.cpp \
    src/ai/HeuristicEngine.cpp \
    src/ai/PredictionEngine.cpp \
    src/planning/*.cpp \
    $(pkg-config --cflags --libs raylib) \
    -o debug_window

ln -sf heist_gui heist_game_ai

echo "✅ Build successful!"
echo ""
echo "🚀 To run the game:"
echo "   ./heist_game_ai"
