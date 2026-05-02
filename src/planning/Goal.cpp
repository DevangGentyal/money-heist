#include "Goal.h"
#include <cmath>

bool Goal::isSatisfied(const Grid3D& grid, 
                       const Position& agentPos,
                       const Position& otherAgentPos) const {
    switch (type) {
        case Type::CATCH:
            return checkCatch(grid, agentPos, otherAgentPos);
        case Type::SAME_CELL:
            return checkSameCell(agentPos, otherAgentPos);
        case Type::SAME_FLOOR:
            return checkSameFloor(agentPos, otherAgentPos);
        case Type::REACH_VAULT:
            return checkReachVault(grid, agentPos);
        case Type::COLLECT_VAULT:
            return checkReachVault(grid, agentPos);  // Same as reach for now
        case Type::REACH_EXIT:
            return checkReachExit(grid, agentPos);
        case Type::ESCAPE:
            return checkReachExit(grid, agentPos);
        case Type::MOVE_TO:
            return checkAtTarget(agentPos);
        case Type::GO_TO_FLOOR:
            return agentPos.z == targetFloor;
        case Type::AVOID_POLICE:
            // This is complex - handled by heuristic/danger evaluation
            return false;
        case Type::PATROL:
            return true;  // Always achievable
        default:
            return false;
    }
}

bool Goal::checkCatch(const Grid3D& grid, 
                     const Position& police, 
                     const Position& robber) const {
    return police == robber;
}

bool Goal::checkSameCell(const Position& p1, const Position& p2) const {
    return p1 == p2;
}

bool Goal::checkSameFloor(const Position& p1, const Position& p2) const {
    return p1.z == p2.z;
}

bool Goal::checkReachVault(const Grid3D& grid, const Position& pos) const {
    return pos == grid.getVaultPos();
}

bool Goal::checkReachExit(const Grid3D& grid, const Position& pos) const {
    return pos == grid.getExitPos();
}

bool Goal::checkAtTarget(const Position& pos) const {
    return pos == targetPos;
}
