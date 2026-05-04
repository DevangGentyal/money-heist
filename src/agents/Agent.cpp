#include "Agent.h"

Agent::Agent(const Position& startPos, AgentRole r) 
    : pos(startPos), lastPos(startPos), role(r), stepsTaken(0) {}

bool Agent::canMoveTo(const Position& target, const Grid3D& grid) const {
    return grid.isValid(target) && !grid.isWall(target);
}
