#include "Precondition.h"
#include "../ai/AStar3D.h"

bool Precondition::isSatisfied(const Grid3D& grid,
                              const Position& agentPos,
                              const Position& otherAgentPos,
                              bool robberHasVault) const {
    switch (type) {
        case Type::SAME_FLOOR:
            return checkSameFloor(agentPos, otherAgentPos);
        case Type::SAME_CELL:
            return checkSameCell(agentPos, otherAgentPos);
        case Type::PATH_EXISTS:
            return checkPathExists(grid, agentPos, otherAgentPos);
        case Type::AT_POSITION:
            return checkAtPosition(agentPos, pos1);
        case Type::AT_STAIRS:
            return checkAtStairs(grid, agentPos);
        case Type::AT_ELEVATOR:
            return checkAtElevator(grid, agentPos);
        case Type::HAS_VAULT:
            return robberHasVault;
        case Type::NOT_IN_ALERT_ZONE:
            return !grid.isAlertZone(agentPos);
        case Type::GOAL_SATISFIED:
            return true;  // Meta-precondition, handled externally
        default:
            return false;
    }
}

bool Precondition::checkSameFloor(const Position& p1, const Position& p2) const {
    return p1.z == p2.z;
}

bool Precondition::checkSameCell(const Position& p1, const Position& p2) const {
    return p1 == p2;
}

bool Precondition::checkPathExists(const Grid3D& grid, 
                                  const Position& p1, 
                                  const Position& p2) const {
    // Quick validation: both positions are valid
    return grid.isValid(p1) && grid.isValid(p2) && 
           !grid.isWall(p1) && !grid.isWall(p2);
}

bool Precondition::checkAtPosition(const Position& p1, const Position& p2) const {
    return p1 == p2;
}

bool Precondition::checkAtStairs(const Grid3D& grid, const Position& pos) const {
    return grid.getCell(pos) == CellType::STAIRS;
}

bool Precondition::checkAtElevator(const Grid3D& grid, const Position& pos) const {
    return grid.getCell(pos) == CellType::ELEVATOR;
}
