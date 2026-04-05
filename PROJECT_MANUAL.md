# Money Heist 3D - AI Project Manual

## 1. Project Overview
**Money Heist 3D** is an advanced Artificial Intelligence course project developed entirely in standard C++. It transforms a traditional 2-Dimensional grid pathfinding game into a complex 3-Dimensional Stealth-Hunting engine. 

You control the **Robber (`R`)**, whose objective is to navigate multiple building floors (Z-axis), locate the **Vault (`$$`)**, and escape through the **Exit (`[]`)** while surviving a constant manhunt by an AI-driven Police force and avoiding dynamic CCTV grids.

---

## 2. Artificial Intelligence Concepts Implemented

To meet BTech algorithmic criteria, the project utilizes several state-of-the-art computational theories and AI logic systems:

### A. 3-Dimensional A* Pathfinding Algorithm (Heuristic Search)
The Police (`P`) do not randomly wander; they use **A* Search**, an informed graph traversal algorithm, to calculate the absolute shortest path to your location across multiple floors.
- **The Graph**: The 3D grid `map[z][y][x]` acts as mathematical nodes. Edges connect adjacent horizontal blocks, and "Stair" nodes (`^^` & `vv`) connect vertical `Z` layers.
- **The Equation**: The AI calculates `f(n) = g(n) + h(n)` for every block. 
  - `g(n)`: The movement cost from the Police's start position to the current block.
  - `h(n)`: The **3D Manhattan Distance Heuristic**, mathematically defined in our engine as `abs(dx) + abs(dy) + (abs(dz) * floor_penalty)`.
- **Behavior**: If you are on Floor 3 and the Police are on Floor 1, A* evaluates millions of micro-paths and successfully deduces that routing to the Staircase node on Floor 1 is mathematically cheaper to reach you than walking into walls, allowing inter-floor hunting.

### B. Bresenham's Line Algorithm (Raycasting & Line of Sight)
Instead of static "distance radius" checks which can look straight through solid walls, our **CCTV (`C`)** network utilizes a raycasting module governed by **Bresenham's Line Algorithm**.
- **How it works**: When verifying if the camera sees you, Bresenham's algorithm draws a continuous mathematical vector between the CCTV lens `(x1, y1)` and the Robber `(x2, y2)`.
- **Wall Penetration Rejection**: The algorithm steps through every single grid block along that vector line. If it encounters a Wall block (`##`), the algebraic loop intentionally breaks, returning `false` (meaning the camera is physically blocked from seeing you).

### C. Finite State Machines (FSM)
To simulate actual "Agent Intelligence", the Police operate on a Finite State Machine rather than fixed code loops. 
- **States**: The AI toggles dynamically between `PATROL`, `ALERT`, and `CHASE` modes.
- **Transitions**: If the CCTV raycast returns `true` (Robber spotted), the system universally feeds the coordinates to the Police AI network, shifting their state to `ALERT`. In Hard difficulty, this allows them to perform **two physical A* movements per turn**, doubling their speed strictly because their "Adrenaline/Alert state" has been elevated.

---

## 3. Engineering & 3D Architectural Complexity

### True Z-Axis Grid Structuring
Most terminal games use a simplistic `vector<string>`. This engine required upgrading the architecture to a true multidimensional tensor matrix: `vector<vector<vector<char>>>`. This structural shift impacts all functions—bounds checking, rendering, and entity validation now securely compute an extra `Z` coordinate simultaneously.

### Sub-Buffer Engine Rendering (Pure C++)
Because terminal rendering flickers when using raw `system("clear")`, the User Interface prints raw **ANSI Escape Codes** directly into the console stream. 
Instead of just printing letters, the engine pushes strings like `\033[31m` (Hex color codes) to render items. By employing `\033[2J\033[1;1H`, the cursor is instantly teleported back to the top-left of the terminal, allowing us to overwrite the grid seamlessly at what feels like 60 FPS without ever relying on an external UI library (like OpenGL). 
We also compute adjacent memory matrices to generate a real-time **Minimap layout** alongside your current visual plane.

---

## 4. How to Play & Controls

### Booting the Game
1. Compile the engine: 
   `clang++ -std=c++17 main.cpp GameEngine.cpp Grid.cpp GridDisplay.cpp Robber.cpp PoliceAI.cpp CCTV.cpp AStar.cpp -o heist`
2. Run the executable: 
   `./heist`

### Reading the Terminal
* `R` -> **Robber** (You)
* `P` -> **Police** (Hunting AI)
* `C` -> **CCTV Camera** (Raycasting Vision)
* `$$` -> **Vault** (Your Objective)
* `[]` -> **Exit** (Escape Point after securing Vault)
* `##` -> Solid Walls
* `^^` -> Stairs Up
* `vv` -> Stairs Down

### Physical Controls
Type the key and press Enter:
- **`w` `a` `s` `d`** : Standard spatial movement (Up, Left, Down, Right).
- **`u`** : Travel **UP** one floor. (You must be firmly standing on `^^`).
- **`j`** : Travel **DOWN** one floor. (You must be firmly standing on `vv`).
- **`q`** : Quit application entirely.

---
*Developed for BTech AI coursework.*
