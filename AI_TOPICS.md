# AI Topics Map (Problem -> Technique -> Implementation)

This project is now organized around explicit AI concepts in the gameplay loop.

## 1) Dynamic A* Search
- Problem handled:
  - Re-planning every turn in a changing world (robber movement, alert state, floor transitions).
- Technique:
  - Dynamic repeated shortest-path planning using A* at each decision step.
- Where implemented:
  - src/ai/AStar3D.cpp
  - src/agents/PoliceAI.cpp
  - src/agents/RobberAI.cpp
- Notes:
  - During alert turns, police do an explicit 2-step capture check using A* path length before normal movement.

## 2) Goal Stack Planning
- Problem handled:
  - Police must prioritize intermediate tactical goals before final chase goals.
- Technique:
  - Stack of goals (LIFO): transition chokepoint, predicted interception point, robber direct target, etc.
- Where implemented:
  - src/agents/PoliceAI.cpp (computeInterceptPath)
- Notes:
  - If chase is not applicable yet, guard/hold goals are selected instead.

## 3) STRIPS-style Action Selection
- Problem handled:
  - Selecting valid action mode from world predicates.
- Technique:
  - STRIPS-like precondition checks over predicates:
    - same-floor?
    - vault-collected?
    - on-vault-floor?
  - Resulting applicable actions:
    - pursueRobber
    - guardVault
    - holdPosition
- Where implemented:
  - src/agents/PoliceAI.cpp (evaluateActions)

## 4) Custom Heuristic + Domain Logic
- Problem handled:
  - Path costs should reflect game semantics (risk zones, vertical cost, transition exposure/control).
- Technique:
  - Weighted heuristic terms, adjusted by difficulty.
- Where implemented:
  - src/ai/HeuristicEngine.cpp
  - src/ai/HeuristicEngine.h
- Notes:
  - Combines distance, police/robber risk, CCTV penalty, alert penalty, and vertical/transition terms.

## 5) Alert-State Temporal Reasoning
- Problem handled:
  - Temporary speed/power boost after alerts.
- Technique:
  - Turn-bounded boost with capability switching (2-step police phase).
- Where implemented:
  - src/core/GameEngineGUI.cpp (policeBoostTurnsRemaining, policeDoubleStepActive)
  - src/rules/RuleEngine.cpp

## 6) Multi-floor Transition Constraints
- Problem handled:
  - Avoid unrealistic vertical movement and enforce directional stairs/elevator semantics.
- Technique:
  - Action preconditions for vertical edges and transition permissions.
- Where implemented:
  - src/grid/Grid3D.cpp (getNeighbors)
  - src/core/GameEngineGUI.cpp (shouldPoliceTransitionFloors)

## Quick Problem-to-Technique Table

| Problem | Technique | Primary File |
|---|---|---|
| Re-plan chase each turn | Dynamic A* | src/ai/AStar3D.cpp |
| Pick tactical intermediate goals | Goal Stack Planning | src/agents/PoliceAI.cpp |
| Decide patrol/guard/chase mode | STRIPS-style action preconditions | src/agents/PoliceAI.cpp |
| Score safe vs risky paths | Custom weighted heuristic | src/ai/HeuristicEngine.cpp |
| Make alerts change pursuit dynamics | Temporal rule state | src/core/GameEngineGUI.cpp |
| Respect 3D transition rules | Constrained graph edges | src/grid/Grid3D.cpp |
