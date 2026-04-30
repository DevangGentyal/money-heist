# 🎮 Money Heist 3D - GUI Edition Setup Guide

Your GUI version is **READY TO RUN**! 🚀

## ⚡ Quick Start

```bash
cd /Users/darshan/Documents/Studies/College/AI/CP/heist-game-ai
./heist_gui
```

That's it! The game will launch with a beautiful Raylib GUI window.

---

## 📋 What Changed from Terminal Version

| Feature | Terminal Version | GUI Version |
|---------|------------------|-------------|
| **Graphics** | ASCII Terminal | Raylib Graphics |
| **Window** | Console | Interactive GUI |
| **Colors** | Terminal Colors | Full RGB with Gradients |
| **Interactions** | Text Input | Real-time Input Handling |
| **Performance** | Limited | 60 FPS Smooth |
| **Visuals** | Basic Symbols | Detailed Cell Rendering |

---

## 🎮 Gameplay (GUI Controls)

### In-Game Controls
```
W     - Move up (on current floor)
S     - Move down (on current floor)
A     - Move left (on current floor)
D     - Move right (on current floor)
Q     - Go down one floor
E     - Go up one floor
ENTER - Confirm your move
ESC   - Quit game
```

### Game Flow
1. **Terminal Menu**: Choose difficulty (Easy/Normal/Hard)
2. **Terminal Menu**: Choose role (Robber/Police)
3. **GUI Window Opens**: Beautiful 1400×900 window with:
   - **Left Side**: 15×15 Grid showing current floor
   - **Right Side**: Status panel, objectives, legend, controls
4. **Play**: Make moves with WASD/QE, confirm with ENTER
5. **AI Responds**: Police use predictive interception
6. **Win/Lose**: Game ends when robber escapes or is caught

---

## 🎨 GUI Features

### Main Grid Display
- **Color-coded cells** for easy identification
- **Real-time agent positions** (green R for robber, red P for police)
- **Zone highlighting** (CCTV zones in purple, Alert zones in orange)
- **Smooth grid rendering** with borders

### Right Sidebar (480px wide)
- **Status Panel**: Turn counter, current status
- **Objective Panel**: Current goal and prediction info
- **Legend**: 8 items showing what each symbol/color means
- **Controls Panel**: Reminder of keyboard controls

### Dynamic UI Elements
- **Status Updates**: Real-time game state messages
  - "🕵️ All Clear - Moving Safely"
  - "📹 CCTV Triggered - Evasion Mode"
  - "🚨 ALERT! Police Hunting!"
  - "✓ HEIST SUCCESSFUL!"
  - "✗ CAUGHT! Police Win"

---

## 🔧 Building from Scratch

### If You Need to Rebuild

```bash
cd /Users/darshan/Documents/Studies/College/AI/CP/heist-game-ai

# Full build command (all systems)
clang++ -std=c++17 -Wall -Wextra \
    src/gui_main.cpp \
    src/core/GameEngineGUI.cpp \
    src/grid/Grid3D.cpp \
    src/agents/Agent.cpp \
    src/agents/RobberAI.cpp \
    src/agents/PoliceAI.cpp \
    src/ai/AStar3D.cpp \
    src/ai/HeuristicEngine.cpp \
    src/ai/PredictionEngine.cpp \
    src/rules/RuleEngine.cpp \
    src/rendering/RaylibRenderer.cpp \
    $(pkg-config --cflags --libs raylib) \
    -o heist_gui
```

### Or Use the Build Script

```bash
chmod +x build_gui.sh
./build_gui.sh
```

---

## 📦 Dependencies

### Raylib
- **Version**: 5.5 (installed)
- **Library**: Modern, lightweight 2D graphics
- **Why**: Perfect for this type of game

Verify installation:
```bash
pkg-config --cflags --libs raylib
```

---

## 📁 File Structure (GUI Components)

```
heist-game-ai/
├── src/
│   ├── gui_main.cpp                 # GUI entry point
│   ├── rendering/
│   │   ├── RaylibRenderer.h         # Main graphics class
│   │   └── RaylibRenderer.cpp       # Implementation (460 lines)
│   └── core/
│       ├── GameEngineGUI.h          # GUI game engine
│       └── GameEngineGUI.cpp        # Implementation
└── heist_gui                        # GUI executable ✓
```

---

## 🎯 Difficulty Levels

### Easy (1 Floor)
- 1 Police Officer
- Slower AI (0.7x speed)
- Prediction depth: 6 steps
- Good for learning

### Normal (2 Floors)
- 2 Police Officers
- Balanced AI (1.0x speed)
- Prediction depth: 10 steps
- Challenging gameplay

### Hard (3 Floors)
- 3 Police Officers
- Aggressive AI (1.3x speed, 1.6x on alert)
- Prediction depth: 12-20 steps
- Expert challenge

---

## 🚀 Run Commands

### GUI Version (Recommended! 🎨)
```bash
cd /Users/darshan/Documents/Studies/College/AI/CP/heist-game-ai
./heist_gui
```

### Terminal Version (Still Available 🖥️)
```bash
cd /Users/darshan/Documents/Studies/College/AI/CP/heist-game-ai
./heist
```

---

## 📊 Technical Details

### Graphics Library: Raylib
- **Resolution**: 1400×1024 pixels
- **FPS**: 60 frames per second
- **Color Depth**: 32-bit RGBA
- **Platform**: macOS, Linux, Windows
- **Size**: ~4.2MB library

### Code Size
- **Total Lines**: ~2000 lines C++
- **GUI Code**: ~460 lines (RaylibRenderer)
- **Game Logic**: ~1500 lines
- **AI Systems**: Custom heuristic, prediction, pathfinding

---

## 🏆 Game Objectives

### As Robber (Manual Play)
1. ✓ Navigate to **Vault** (marked `V`)
2. ✓ Grab the loot
3. ✓ Escape to **Exit** (marked `E`)
4. ✓ Avoid **Police** (`P`) and **CCTV Zones** (`C`)
5. ✓ Minimize turns taken

### As Police (Manual Play)
1. ✓ Catch the **AI Robber** before it escapes
2. ✓ Use **predictive pathfinding** to intercept
3. ✓ Hunt efficiently

---

## 💡 Tips & Tricks

### For Robber Victory
- Use stairs/elevators to confuse police
- Avoid CCTV zones (costs more moves)
- Plan multi-floor routes
- Stay away from alert zones

### For Police Victory
- Predict robber's destination (vault or exit)
- Cut off escape routes
- Use multiple officers for flanking
- React faster when CCTV triggers

---

## 🐛 Troubleshooting

### GUI won't open?
- Check Raylib is installed: `pkg-config --cflags --libs raylib`
- Verify permissions: `chmod +x heist_gui`
- Try rebuilding: `./build_gui.sh`

### Slow performance?
- Close other applications
- Check GPU drivers are up to date
- Recent macOS versions prioritize GPU-accelerated rendering

### Controls not responding?
- Click the window to focus it
- Ensure NUM LOCK is off (affects some pads)
- Try pressing ESC and starting fresh

---

## 🎬 Example Gameplay

```
1. Select: Easy Difficulty, Play as Robber
2. Window opens showing:
   - Green R: Your position at (7,7,0)
   - Yellow V: Vault at (7,7,0)
   - Red P: Police at (3,3,0)
   - Blue E: Exit at (1,1,0)
3. You move: W→S→A→D
4. Police hunts predictively
5. You reach vault, move to exit
6. ✓ YOU WIN! (Turn 42)
```

---

## 📚 Learn More

- **A* Pathfinding**: Classic algorithm with 6D movement
- **Heuristic Engine**: Weighted multi-factor scoring
- **Predictive AI**: Game AI interception strategy
- **Raylib**: https://www.raylib.com

---

**Enjoy the game! May your heists be successful! 🏆**

*Powered by Raylib | Built with C++17 | AI-Driven Gameplay*
