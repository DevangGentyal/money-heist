#include "PoliceAI.h"
#include <cstdlib>

PoliceAI::PoliceAI() : Agent({0, 0, 0}), state(AIState::PATROL), lastKnownTarget({0,0,0}) {}

PoliceAI::PoliceAI(Position startPos) : Agent(startPos), state(AIState::PATROL), lastKnownTarget(startPos) {}

bool PoliceAI::takeTurn(Position targetPos, bool isAlerted, Position alertPos, const Grid& grid) {
    if (isAlerted) {
        state = AIState::ALERT;
        lastKnownTarget = alertPos;
    }
    
    // Check if adjacent to catch early
    if (abs(pos.x - targetPos.x) + abs(pos.y - targetPos.y) + abs(pos.z - targetPos.z) <= 1) {
        pos = targetPos; // Catch!
        return true;
    }

    Position goal = targetPos;
    
    // In Hard Mode, we can keep the Alert mechanic (Moving 2 steps instead of 1)
    // But behaviorally, they should ALWAYS hunt using A* Pathfinding.
    state = AIState::CHASE; 

    // AStar CHASE or ALERT routing
    std::vector<Position> path = AStar::findPath(pos, goal, grid);
    if (!path.empty()) {
        pos = path[0];
        return true;
    }

    return false;
}
