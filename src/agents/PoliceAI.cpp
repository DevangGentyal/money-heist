#include "PoliceAI.h"
#include <algorithm>

namespace {
struct ActionApplicability {
    bool pursueRobber;
    bool guardVault;
    bool holdPosition;
};

ActionApplicability evaluateActions(const Grid3D& grid,
                                    const Position& policePos,
                                    const Position& robberPos,
                                    bool vaultCollected) {
    bool sameFloor = policePos.z == robberPos.z;
    bool isActive = vaultCollected || sameFloor;
    bool onVaultFloor = policePos.z == grid.getVaultPos().z;

    ActionApplicability a{};
    a.pursueRobber = isActive;
    a.guardVault = !isActive && onVaultFloor && !vaultCollected;
    a.holdPosition = !a.pursueRobber && !a.guardVault;
    return a;
}

Position findTransitionTargetTowardFloor(const Grid3D& grid,
                                        const Position& from,
                                        int targetFloor) {
    if (from.z == targetFloor) {
        return from;
    }

    CellType desired = targetFloor > from.z ? CellType::STAIRS : CellType::ELEVATOR;
    int bestDist = 1000000;
    Position best = from;

    for (int y = 1; y < grid.getHeight() - 1; ++y) {
        for (int x = 1; x < grid.getWidth() - 1; ++x) {
            Position p(x, y, from.z);
            if (grid.getCell(p) != desired) {
                continue;
            }

            int dist = grid.manhattanDistance(from, p);
            if (dist < bestDist) {
                bestDist = dist;
                best = p;
            }
        }
    }

    return best;
}
} // namespace

PoliceAI::PoliceAI(const Position& startPos,
                   PredictionEngine* pred,
                   HeuristicEngine* h,
                   RuleEngine* r)
    : Agent(startPos, AgentRole::POLICE),
      state(PoliceState::PATROL),
      predictor(pred),
      heuristic(h),
      rules(r),
      vaultCollected(false) {}

Position PoliceAI::getNextMove(const Grid3D& grid,
                              const vector<Position>& otherAgents) {
    if (otherAgents.empty()) {
        return pos; // No target
    }
    
    const Position& targetPos = otherAgents[0]; // Robber is first agent
    // Once the vault is taken, the alarm is global.
    bool huntMode = (pos.z == targetPos.z) || vaultCollected;
    bool isDetected = vaultCollected || (huntMode && (grid.manhattanDistance(pos, targetPos) < 12));
    
    updateState(grid, targetPos, isDetected);
    return computeInterceptPath(grid, targetPos);
}

void PoliceAI::updateState(const Grid3D& grid, const Position& targetPos, bool isDetected) {
    bool huntMode = (pos.z == targetPos.z) || vaultCollected;
    if (!huntMode) {
        state = PoliceState::PATROL;
        return;
    }

    int dist = grid.manhattanDistance(pos, targetPos);
    
    if (dist < 2) {
        state = PoliceState::CHASE;
    } else if (isDetected && predictor) {
        state = PoliceState::INTERCEPT;
    } else if (isDetected) {
        state = PoliceState::ALERT;
    } else {
        state = PoliceState::PATROL;
    }
}

Position PoliceAI::computeInterceptPath(const Grid3D& grid, const Position& targetPos) {
    ActionApplicability actions = evaluateActions(grid, pos, targetPos, vaultCollected);

    // Goal Stack Planning (top/back is current planning focus)
    vector<Position> goalStack;
    if (actions.holdPosition) {
        goalStack.push_back(pos);
    } else if (actions.guardVault) {
        goalStack.push_back(grid.getVaultPos());
    } else if (actions.pursueRobber) {
        goalStack.push_back(targetPos);

        if (pos.z != targetPos.z) {
            goalStack.push_back(findTransitionTargetTowardFloor(grid, pos, targetPos.z));
        } else if (state == PoliceState::INTERCEPT && predictor) {
            Position predicted = predictTargetPosition(grid, targetPos);
            // Only use prediction if it's within a very tight range around the robber
            if (grid.manhattanDistance(predicted, targetPos) <= 2) {
                goalStack.push_back(predicted);
            }
        }
    }

    vector<Position> robberAsThreat{targetPos};
    bool canTransition = actions.pursueRobber ? vaultCollected : false;

    while (!goalStack.empty()) {
        Position goal = goalStack.back();
        goalStack.pop_back();

        vector<Position> path;
        if (heuristic) {
            path = AStar3D::findPathWithHeuristic(pos,
                                                  goal,
                                                  grid,
                                                  *heuristic,
                                                  robberAsThreat,
                                                  false,
                                                  canTransition);
        } else {
            path = AStar3D::findPath(pos, goal, grid, canTransition);
        }

        if (!path.empty() && path.size() > 1) {
            return path[1];
        }
    }

    return pos;
}

Position PoliceAI::predictTargetPosition(const Grid3D& grid, const Position& targetPos) {
    if (!predictor) return targetPos;
    
    // Get prediction depth from rules
    int depth = 10;
    if (rules) {
        depth = rules->getPredictionDepth();
    }
    
    Position predictedGoal = vaultCollected ? grid.getExitPos() : grid.getVaultPos();
    Position interception = predictor->getInterceptionPoint(targetPos,
                                                            predictedGoal,
                                                            min(depth / 2, 2)); // Aggressively shorter prediction

    // Avoid selecting far strategic points that make police camp objectives.
    if (grid.manhattanDistance(targetPos, interception) > 3) {
        return targetPos;
    }

    return interception;
}
