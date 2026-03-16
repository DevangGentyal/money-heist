<div align="center">
  <h1>💰 Money Heist</h1>
  <p><b>Terminal Prototype</b></p>
  <p>A C++ grid-based AI game where a Robber tries to escape a Vault while being hunted by an A* Pathfinding Police.</p>
</div>

---

## 📂 Project Structure

All files are located in the root directory. The project is modularly divided into the following components:

- **`main.cpp`** — The entry point of the game.
- **`GameEngine`** (`.h` / `.cpp`) — Manages the game loop, turns, and win/loss conditions.
- **`Grid`** (`.h` / `.cpp`) — Stores the 2D map, walls, and initial entity positions.
- **`GridDisplay`** (`.h` / `.cpp`) — Responsible solely for rendering the grid and game state in the terminal.
- **`Agent`** (`.h`) — Base class for tracking coordinates `(x, y)`.
- **`Robber`** (`.h` / `.cpp`) — Handles user-controlled movement and vault collection status.
- **`PoliceAI`** (`.h` / `.cpp`) — Handles AI behavior and pathfinding usage to hunt the Robber.
- **`AStar`** (`.h` / `.cpp`) — The core pathfinding algorithm using Manhattan distance to calculate shortest paths.

---

## 🎮 How to Play

### 1. Compile the Game
Make sure you have `clang++` installed, then run the exact command below in your terminal:
```bash
clang++ main.cpp GameEngine.cpp Grid.cpp GridDisplay.cpp Robber.cpp PoliceAI.cpp AStar.cpp -o heist
```

### 2. Run the Game
Execute the compiled binary:
```bash
./heist
```

### 3. Controls
You control the **Robber (`R`)**. Move using the following keys, followed by the `Enter` key:
- `w` : Move Up
- `s` : Move Down
- `a` : Move Left
- `d` : Move Right

### 4. Objectives
- **Robber (`R`)**: Reach the Vault (`V`), then escape through the Exit (`E`).
- **Police (`P`)**: The AI will automatically compute the shortest path using A* Search to catch you before you escape.

*Beware of Walls (`#`). You can only step on empty spaces (`.`).*
