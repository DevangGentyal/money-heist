// RobberAIPlanner.cpp
#include "RobberAIPlanner.h"

#include "../ai/AStar3D.h"
#include <algorithm>
#include <cmath>

namespace {
GoalEntry makeEntry(const string& expression, bool isOperator, const string& opName,
                    const vector<string>& preconditions, const vector<string>& effects) {
    GoalEntry entry;
    entry.goalExpression = expression;
    entry.isOperator = isOperator;
    entry.operatorName = opName;
    entry.preconditions = preconditions;
    entry.effects = effects;
    return entry;
}
}

RobberAIPlanner::RobberAIPlanner()
    : heuristic(nullptr), rules(nullptr), hasVault(false), initialized(false) {}

void RobberAIPlanner::initializePlan(const WorldState& world) {
    goalStack.clear();
    // Push escape (bottom)
    GoalEntry escape = makeEntry("escape(Robber,Exit)", false, "", {}, {});
    goalStack.push(escape);
    // Push steal (top)
    GoalEntry steal = makeEntry("steal(Robber,Vault)", false, "", {}, {});
    goalStack.push(steal);
    // Immediately decompose steal into operator block (top-most: MOVE operator active)
    // MOVE(Robber, Vault) operator
    GoalEntry mv = makeOperator("MOVE", {"pathExists(Robber,VaultPos)", "NOT wall(VaultPos)"}, {"Robber.pos=VaultPos"});
    mv.goalExpression = "MOVE(Robber, Vault)";
    mv.isOperator = true;
    goalStack.push(mv);
    // compound precondition
    GoalEntry pre = makeGoal("atVault(Robber) ^ vaultClear(Vault)");
    goalStack.push(pre);
    // STEAL operator (explicit)
    GoalEntry stealOp = makeOperator("STEAL", {"Robber.pos == Vault.pos", "NOT vaultStolen"}, {"vaultStolen=true", "Robber.hasVault=true"});
    stealOp.goalExpression = "STEAL(Robber, Vault)";
    stealOp.isOperator = true;
    goalStack.push(stealOp);
    vaultPos = world.vaultPos;
    exitPos = world.exitPos;
    hasVault = world.robberHasVault;
    initialized = true;
}

GoalEntry RobberAIPlanner::makeGoal(const string& expression, const vector<string>& preconditions, const vector<string>& effects) {
    return makeEntry(expression, false, "", preconditions, effects);
}

GoalEntry RobberAIPlanner::makeOperator(const string& name, const vector<string>& preconditions, const vector<string>& effects) {
    return makeEntry(name, true, name, preconditions, effects);
}

void RobberAIPlanner::ensureInitialStack(WorldState& world) {
    if (!initialized || goalStack.isEmpty()) {
        initializePlan(world);
    }
}

Position RobberAIPlanner::executeMove(const Position& target, WorldState& world, GoalType goalType) {
    if (!world.grid) return world.robberPos;

    WorldState planningWorld = world;
    planningWorld.activeGoalType = goalType;
    planningWorld.activeTargetPos = target;

    vector<Position> police = world.policePositions;
    vector<Position> path;
    if (heuristic && world.grid && world.grid->getDepth() > 0) {
        RuleEngine fallbackRules(1);
        RuleEngine& activeRules = rules ? *rules : fallbackRules;
        path = AStar3D::findPathWithHeuristic(world.robberPos, target, *world.grid, *heuristic,
                                              planningWorld, activeRules, goalType,
                                              police, true, true);
    } else {
        path = AStar3D::findPath(world.robberPos, target, *world.grid, goalType, true);
    }

    // capture A* trace for debug snapshot
    try {
        lastAstarTrace = AStar3D::getLastSearchTrace();
        hasLastAstarTrace = true;
    } catch (...) {
        hasLastAstarTrace = false;
    }

    if (path.size() > 1) {
        world.robberPos = path[1];
    }
    return world.robberPos;
}

bool RobberAIPlanner::executeTop(WorldState& world) {
    if (goalStack.isEmpty()) return false;

    GoalEntry top = goalStack.peek();
    if (!top.isOperator) {
        if (top.goalExpression == "steal(Robber,Vault)" && world.robberPos == world.vaultPos && !world.vaultStolen) {
            goalStack.markComplete();
            return true;
        }
        if (top.goalExpression == "escape(Robber,Exit)" && world.robberPos == world.exitPos && world.robberHasVault) {
            goalStack.markComplete();
            return true;
        }
    }
    if (top.isOperator) {
        if (top.operatorName == "MOVE") {
            if (world.robberPos == world.vaultPos) {
                goalStack.markComplete();
                return true;
            }
            Position target = top.goalExpression.find("Alert") != string::npos ? world.alertPos : (world.robberHasVault ? world.exitPos : world.vaultPos);
            world.robberPos = executeMove(target, world, world.robberHasVault ? GoalType::ESCAPE_TO_EXIT : GoalType::REACH_VAULT);
            if (world.robberPos == target) {
                // Check if reached stairs to auto-transition floors
                // Only allow floor transitions after vault is stolen
                if (world.vaultStolen && world.grid) {
                    CellType cellType = world.grid->getCell(world.robberPos);
                    if (cellType == CellType::STAIRS) {
                        if (target.z < world.robberPos.z) {
                            if (world.robberPos.z > 0) world.robberPos.z--;  // Move DOWN one floor
                        } else {
                            if (world.robberPos.z < world.grid->getDepth() - 1) world.robberPos.z++;  // Move UP one floor
                        }
                    }
                }
                goalStack.markComplete();
            } else {
                goalStack.markPerforming();
            }
        } else if (top.operatorName == "MOVE_FLOOR") {
            world.robberPos.z = world.activeTargetFloor;
            goalStack.markComplete();
        } else if (top.operatorName == "STEAL") {
            if (world.robberPos == world.vaultPos) {
                world.vaultStolen = true;
                world.robberHasVault = true;
                goalStack.markComplete();
            }
        } else if (top.operatorName == "ESCAPE") {
            if (world.robberPos == world.exitPos && world.robberHasVault) {
                world.robberWon = true;
                goalStack.markComplete();
            }
        } else if (top.operatorName == "DISTRACT") {
            world.alertTriggered = true;
            goalStack.markComplete();
        }
        return true;
    }

    if (top.goalExpression == "steal(Robber,Vault)") {
        // Decompose
        goalStack.push(makeGoal("atVault(Robber) ^ vaultClear(Vault)"));
        GoalEntry stealOp = makeOperator("STEAL", {"Robber.pos == Vault.pos", "NOT vaultStolen"}, {"vaultStolen=true", "Robber.hasVault=true"});
        stealOp.goalExpression = "STEAL(Robber, Vault)";
        stealOp.isOperator = true;
        goalStack.push(stealOp);
        return true;
    }

    if (top.goalExpression == "atVault(Robber) ^ vaultClear(Vault)") {
        GoalEntry mv = makeOperator("MOVE", {"pathExists(Robber,VaultPos)", "NOT wall(VaultPos)"}, {"Robber.pos=VaultPos"});
        mv.goalExpression = "MOVE(Robber, Vault)";
        mv.isOperator = true;
        goalStack.push(mv);
        return true;
    }

    if (top.goalExpression == "escape(Robber,Exit)") {
        goalStack.push(makeGoal("atExit(Robber) ^ hasVault(Robber)"));
        GoalEntry escOp = makeOperator("ESCAPE", {"Robber.pos == Exit.pos", "Robber.hasVault == true"}, {"RobberWins=true"});
        escOp.goalExpression = "ESCAPE(Robber)";
        escOp.isOperator = true;
        goalStack.push(escOp);
        return true;
    }

    if (top.goalExpression == "atExit(Robber) ^ hasVault(Robber)") {
        if (world.robberPos.z != world.exitPos.z) {
            GoalEntry mvf = makeOperator("MOVE_FLOOR", {"transitionCellExists", "floorTransitionAllowed"}, {"Robber.floor=ExitFloor"});
            mvf.goalExpression = string("MOVE_FLOOR(Robber, ") + to_string(world.exitPos.z) + ")";
            mvf.isOperator = true;
            goalStack.push(mvf);
        }
        GoalEntry mv = makeOperator("MOVE", {"pathExists(Robber,ExitPos)", "NOT wall(ExitPos)"}, {"Robber.pos=ExitPos"});
        mv.goalExpression = "MOVE(Robber, Exit)";
        mv.isOperator = true;
        goalStack.push(mv);
        return true;
    }

    return false;
}

Position RobberAIPlanner::runTurn(WorldState& world) {
    ensureInitialStack(world);
    goalStack.finalizeCompleted();

    for (int iteration = 0; iteration < 8 && !goalStack.isEmpty(); ++iteration) {
        goalStack.finalizeCompleted();
        if (goalStack.isEmpty()) {
            break;
        }

        GoalEntry top = goalStack.peek();
        if (top.status == GoalEntry::COMPLETED) {
            continue;
        }
        // If top was performing previously, finalize it now
        if (top.status == GoalEntry::PERFORMING) {
            goalStack.markComplete();
            goalStack.finalizeCompleted();
            continue;
        }
        if (!top.isOperator) {
            if (top.goalExpression == "steal(Robber,Vault)" && world.robberPos == world.vaultPos && !world.vaultStolen) {
                goalStack.markComplete();
                goalStack.finalizeCompleted();
                continue;
            }
            if (top.goalExpression == "escape(Robber,Exit)" && world.robberPos == world.exitPos && world.robberHasVault) {
                goalStack.markComplete();
                goalStack.finalizeCompleted();
                continue;
            }
        }

        if (!executeTop(world)) {
            break;
        }

        if (!goalStack.isEmpty()) {
            GoalEntry updatedTop = goalStack.peek();
            if (updatedTop.status == GoalEntry::COMPLETED) {
                goalStack.finalizeCompleted();
                continue;
            }
            if (updatedTop.status == GoalEntry::PERFORMING) {
                break;
            }
        }
    }

    return world.robberPos;
}

string RobberAIPlanner::getPlanningDashboard() const {
    return goalStack.debugSummary();
}

float RobberAIPlanner::evaluateDanger(const Grid3D& grid, const Position& pos, const Position& policePos) const {
    float dist = abs(pos.x - policePos.x) + abs(pos.y - policePos.y) + abs(pos.z - policePos.z) * 3.0f;
    float dangerFromPolice = 1.0f / (dist + 1.0f);
    dangerFromPolice = dangerFromPolice * dangerFromPolice;
    float dangerFromCCTV = grid.isCCTVZone(pos) ? 0.5f : 0.0f;
    float dangerFromAlert = grid.isAlertZone(pos) ? 2.0f : 0.0f;
    return dangerFromPolice + dangerFromCCTV + dangerFromAlert;
}
