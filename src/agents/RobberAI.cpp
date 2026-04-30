#include "RobberAI.h"
#include <cmath>
#include <algorithm>

RobberAI::RobberAI(const Position& startPos, HeuristicEngine* h)
    : Agent(startPos, AgentRole::ROBBER), 
      state(RobberState::HUNTING_VAULT),
      hasVault(false),
      heuristic(h),
      turnsWaiting(0),
      lastPos(startPos) {}

Position RobberAI::getNextMove(const Grid3D& grid, 
                               const vector<Position>& policePositions) {
    updateState(grid, policePositions);
    return computeSafePath(grid, policePositions);
}

void RobberAI::updateState(const Grid3D& grid, const vector<Position>& policePositions) {
    // Check if cornered
    float minDist = 1000.0f;
    for (const auto& pPos : policePositions) {
        int dist = grid.manhattanDistance(pos, pPos);
        minDist = min(minDist, (float)dist);
    }
    
    if (minDist < 2) {
        state = RobberState::CORNERED;
        return;
    }
    
    if (minDist < 4) {
        state = RobberState::EVASION;
    } else if (hasReachedVault(grid)) {
        if (!hasVault) {
            hasVault = true;
        }
        state = RobberState::ESCAPING;
    } else {
        state = RobberState::HUNTING_VAULT;
    }
}

Position RobberAI::computeSafePath(const Grid3D& grid, 
                                   const vector<Position>& policePositions) {
    Position goal;
    
    if (state == RobberState::ESCAPING || hasVault) {
        goal = grid.getExitPos();
    } else {
        goal = grid.getVaultPos();
    }
    
    vector<Position> path;
    
    if (heuristic) {
        path = AStar3D::findPathWithHeuristic(pos, goal, grid, *heuristic, 
                                            policePositions, true);
    } else {
        path = AStar3D::findPath(pos, goal, grid);
    }
    
    if (!path.empty() && path.size() > 1) {
        Position nextStep = path[1];
        float currentDanger = evaluateDanger(pos, policePositions);
        float nextDanger = evaluateDanger(nextStep, policePositions);

        // Stuck prevention: Decay danger threshold as we wait
        float dangerThreshold = 0.5f + (turnsWaiting * 0.5f);

        // In high-risk states, avoid stepping into substantially more dangerous tiles.
        if ((state == RobberState::EVASION || state == RobberState::CORNERED || hasVault) &&
            nextDanger > currentDanger + dangerThreshold) {
            
            vector<Position> candidates = grid.getNeighbors(pos, true);
            Position safest = pos;
            float safestDanger = currentDanger;

            for (const auto& candidate : candidates) {
                float d = evaluateDanger(candidate, policePositions);
                if (d < safestDanger) {
                    safestDanger = d;
                    safest = candidate;
                }
            }

            if (safest == pos) {
                turnsWaiting++;
            } else {
                turnsWaiting = 0;
            }

            return safest;
        }

        turnsWaiting = 0;
        return nextStep;
    }
    
    turnsWaiting++;
    return pos;
}

float RobberAI::evaluateDanger(const Position& p, 
                              const vector<Position>& policePositions) const {
    float totalDanger = 0.0f;
    
    for (const auto& pPos : policePositions) {
        int dist = abs(p.x - pPos.x) + abs(p.y - pPos.y) + abs(p.z - pPos.z);
        if (dist < 5) {
            totalDanger += pow(1.0f / max(1, dist), 2);
        }
    }
    
    return totalDanger;
}

bool RobberAI::hasReachedVault(const Grid3D& grid) const {
    return pos == grid.getVaultPos();
}

bool RobberAI::hasReachedExit(const Grid3D& grid) const {
    return pos == grid.getExitPos();
}
