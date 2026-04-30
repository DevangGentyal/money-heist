# 🏆 Money Heist 3D: Adaptive Heuristic AI Simulator

A sophisticated 3D multi-level adversarial game featuring AI-driven pathfinding, predictive interception strategies, and dynamic difficulty scaling. Built with custom A* heuristics and intelligent agent behavior.

## AI Concepts Guide

For a direct mapping of game problems to AI techniques and implementation files, see [AI_TOPICS.md](AI_TOPICS.md).

## 🎮 Core Features

### 1️⃣ **3D Grid (Multi-Floor)**
- **State Space**: `(x, y, z)` coordinates with vertical movement
- **3 Floors** at Hard difficulty
- **Vertical Movement**: Stairs (cost 3) and elevators (cost 3)
- **Z-axis Planning**: AI must account for vertical routing costs

### 2️⃣ **Custom Heuristic Engine** ⭐ YOUR MAIN CONTRIBUTION

The HeuristicEngine implements weighted multi-factor scoring:

```
f(n) = g(n) + h(n)

h(n) = w_distance * manhattan(pos, goal)
     + w_police_proximity * risk_from_police
     + w_cctv_penalty * cctv_risk(pos)
     + w_alert_zone_penalty * alert_risk(pos)
     + w_vertical_cost * vertical_movement_cost
```

**Difficulty Adjustments**:
- **Easy**: Lower police risk, minimal CCTV penalty
- **Normal**: Balanced weights
- **Hard**: Aggressive police proximity penalty, high CCTV penalty

### 3️⃣ **Predictive AI Strategy** 🧠

**Police AI doesn't chase—it intercepts:**

```
robber_goal = PREDICT(robber_pos, vault, exit)
robber_path = A*(robber_pos → robber_goal)
intercept_point = robber_path[prediction_depth]
police_goal = A*(police_pos → intercept_point)
```

**Key Advantage**: Police cuts off escape routes rather than chasing current position

### 4️⃣ **Dynamic Difficulty Scaling**

| Event | Effect |
|-------|--------|
| **CCTV Triggered** | Police speed ↑ 1.3x, Prediction depth ↑ |
| **Alert Zone** | Police speed ↑ 1.6x, All-out hunt |
| **High Difficulty** | Prediction depth reaches 20 steps ahead |

### 5️⃣ **Role Flexibility**

```
Player Options:
├─ Play as ROBBER (Manual)
│  └─ AI controls Police (Predictive hunting)
│
└─ Play as POLICE (Manual)
   └─ AI controls Robber (Risk avoidance)
```

## 📁 Project Structure

```
MoneyHeist3D/
│
├── CMakeLists.txt              # Build configuration
│
├── src/
│   ├── main.cpp                # Entry point with role/difficulty selection
│   │
│   ├── core/
│   │   ├── GameEngine.h        # Main game orchestrator
│   │   └── GameEngine.cpp
│   │
│   ├── grid/
│   │   ├── Grid3D.h            # 3D grid with cell types
│   │   └── Grid3D.cpp
│   │
│   ├── agents/
│   │   ├── Agent.h             # Base agent class
│   │   ├── Agent.cpp
│   │   ├── RobberAI.h          # Robber with safety heuristics
│   │   ├── RobberAI.cpp
│   │   ├── PoliceAI.h          # Police with predictive interception
│   │   └── PoliceAI.cpp
│   │
│   ├── ai/
│   │   ├── AStar3D.h           # A* pathfinding with custom heuristics
│   │   ├── AStar3D.cpp
│   │   ├── HeuristicEngine.h   # ⭐ Weighted multi-factor heuristics
│   │   ├── HeuristicEngine.cpp
│   │   ├── PredictionEngine.h  # Path prediction for interception
│   │   └── PredictionEngine.cpp
│   │
│   ├── rules/
│   │   ├── RuleEngine.h        # Dynamic difficulty management
│   │   └── RuleEngine.cpp
│   │
│   └── rendering/
│       ├── Renderer.h          # Terminal-based visualization
│       └── Renderer.cpp
│
├── assets/
│   ├── textures/
│   └── fonts/
│
└── README.md                    # This file
```

## 🚀 Building & Running

### Prerequisites
- C++17 compiler (GCC, Clang, or MSVC)
- CMake 3.10+

### Build
```bash
cd heist-game-ai
mkdir build
cd build
cmake ..
make
```

### Run
```bash
./bin/heist
```

## 🎯 Gameplay

### As Robber (Manual)
1. Navigate to vault (marked `V`)
2. Grab the loot
3. Escape to exit (marked `E`)
4. Avoid police `P` and CCTV zones `C`

**Controls**:
```
WASD  = Move horizontally
Q/E   = Move between floors
Enter = Confirm move
```

### As Police (Manual)
- Control one police unit manually
- Catch the AI robber
- AI robber uses predictive pathfinding (tries to get vault → exit safely)

## 🧠 AI Behavior

### Robber AI States
- **HUNTING_VAULT**: Moving toward vault, using safety heuristics
- **ESCAPING**: Moving toward exit after obtaining vault
- **EVASION**: Police nearby, high-weight police proximity penalty
- **CORNERED**: Police adjacent, emergency escape mode

### Police AI States
- **PATROL**: No detection, random movement
- **ALERT**: Target spotted within distance 8
- **CHASE**: Direct pursuit (distance < 2)
- **INTERCEPT**: Using predictive path interception (main innovation)

## 📊 Heuristic Components

### Distance Penalty
```
score_distance = manhattan(pos, goal)
```

### Police Proximity Risk
```
score_police = {
  < 3 steps: 10 + (3 - dist) * 5
  3-6 steps: 5
  > 6 steps: 0
}
```

### CCTV Penalty
```
score_cctv = {
  within 3 steps: 10 - dist * 2
  beyond 3 steps: 0
}
```

### Vertical Movement Cost
```
score_vertical = |z₁ - z₂| * 3
```

## 🎮 Example Scenarios

### Scenario 1: Easy Difficulty
- **Grid**: 1 floor, 15×15
- **Agents**: 1 Robber, 1 Police
- **Goal**: Test basic pathfinding
- **Police Speed**: 0.7x (slower)
- **Prediction Depth**: 6 steps

### Scenario 2: Hard Difficulty
- **Grid**: 3 floors, 15×15 each
- **Agents**: 1 Robber, 3 Police
- **Goal**: Complex multi-floor heist
- **Triggered**: CCTV and Alert zones
- **Police Speed**: 1.3-1.6x (faster upon alert)
- **Prediction Depth**: 12-20 steps

## 📈 Performance Metrics

The game tracks:
- **Turn Count**: How many turns to complete heist/catch robber
- **Success Rate**: Percentage of games won by player
- **AI Efficiency**: Prediction accuracy and interception success
- **Difficulty Scaling**: How well difficulty adjusts to player skill

## 🔧 Extending the Project

### Adding SFML Visualization
Uncomment in `CMakeLists.txt`:
```cmake
find_package(SFML 2.5 COMPONENTS graphics window system)
target_link_libraries(heist PRIVATE sfml-graphics)
```

### Custom Heuristic Weights
Modify in `src/core/GameEngine.cpp`:
```cpp
HeuristicWeights weights;
weights.w_police_proximity = 0.8f;  // Higher = more police-aware
weights.w_cctv_penalty = 1.5f;      // Higher = more CCTV-avoidant
```

### New Difficulty Levels
Extend `RuleEngine.h` with additional `DifficultyState` enum values

## 📚 References

- **A* Pathfinding**: Classic algorithm with custom heuristic
- **Predictive AI**: Game AI interception strategy
- **Weighted Heuristics**: Multi-factor decision making in game AI
- **3D Grid Navigation**: Z-axis planning with vertical movement costs

## 🏆 What Makes This Special

✅ **Not Basic**: Custom weighted heuristic tailored to game mechanics  
✅ **Advanced**: Predictive interception instead of naive chasing  
✅ **Scalable**: Dynamic difficulty that adapts to game state  
✅ **Flexible**: Play as robber OR police with different AI behaviors  
✅ **Educational**: Clear separation of AI systems (pathfinding, prediction, rules)  

## 📝 Notes

- AI behavior is deterministic for reproducibility
- All calculations use integer arithmetic for stability
- Modular design allows easy addition of new agents and behaviors
- Console rendering provides instant visual feedback

---

**Developed for AI & Competitive Programming Course**

*Custom Heuristic Engine: Where YOUR Intelligence Shines* ⭐
