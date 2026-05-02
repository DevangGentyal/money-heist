#include "RobberAI.h"

RobberAI::RobberAI(const Position& startPos, HeuristicEngine* h)
    : Agent(startPos, AgentRole::ROBBER),
      state(RobberState::HUNTING_VAULT),
      hasVault(false),
      heuristic(h),
            rules(nullptr),
      turnsWaiting(0),
      lastPos(startPos) {
    planner = make_unique<RobberAIPlanner>();
    planner->setHeuristicEngine(h);
    planner->setRuleEngine(rules);
}

Position RobberAI::getNextMove(const Grid3D& grid,
                               const vector<Position>& policePositions) {
    if (!planner) {
        return pos;
    }

    WorldState world;
    world.grid = &grid;
    world.robberPos = pos;
    world.vaultPos = grid.getVaultPos();
    world.exitPos = grid.getExitPos();
    world.alertPos = grid.getAlertZones().empty() ? Position() : grid.getAlertZones().front();
    world.robberHasVault = hasVault;
    world.vaultStolen = hasVault;
    world.alertTriggered = false;
    world.policePositions = policePositions;

    Position nextMove = planner->runTurn(world);
    pos = nextMove;
    hasVault = world.robberHasVault || world.vaultStolen;
    planningDashboard = planner->getPlanningDashboard();
    return nextMove;
}

void RobberAI::updateState(const Grid3D& grid, const vector<Position>& policePositions) {
    float minDist = 1000.0f;
    for (const auto& pPos : policePositions) {
        int dist = grid.manhattanDistance(pos, pPos);
        minDist = min(minDist, (float)dist);
    }

    if (minDist < 2) {
        state = RobberState::CORNERED;
    } else if (minDist < 4) {
        state = RobberState::EVASION;
    } else if (hasReachedVault(grid)) {
        hasVault = true;
        state = RobberState::ESCAPING;
    } else {
        state = RobberState::HUNTING_VAULT;
    }
}

Position RobberAI::computeSafePath(const Grid3D&, const vector<Position>&) { return pos; }

float RobberAI::evaluateDanger(const Position& p, const vector<Position>& policePositions) const {
    float totalDanger = 0.0f;
    for (const auto& pPos : policePositions) {
        int dist = abs(p.x - pPos.x) + abs(p.y - pPos.y) + abs(p.z - pPos.z);
        if (dist < 5) {
            totalDanger += pow(1.0f / max(1, dist), 2);
        }
    }
    return totalDanger;
}

bool RobberAI::hasReachedVault(const Grid3D& grid) const { return pos == grid.getVaultPos(); }

bool RobberAI::hasReachedExit(const Grid3D& grid) const { return pos == grid.getExitPos(); }
