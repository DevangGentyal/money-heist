#include "GoalStackPlanner.h"
#include <algorithm>
#include <sstream>

namespace ai {
namespace planner {

namespace {

string goalTypeToString(GoalType type) {
    switch (type) {
        case GoalType::PROTECT_VAULT: return "PROTECT_VAULT";
        case GoalType::CHASE_ROBBER: return "CHASE_ROBBER";
        case GoalType::RESPOND_ALERT: return "RESPOND_ALERT";
        case GoalType::REACH_VAULT: return "REACH_VAULT";
        case GoalType::ESCAPE_TO_EXIT: return "ESCAPE_TO_EXIT";
        case GoalType::REACH_ALERT_ZONE: return "REACH_ALERT_ZONE";
        case GoalType::MOVE_FLOOR_GOAL: return "MOVE_FLOOR_GOAL";
        default: return "NONE";
    }
}

GoalType goalTypeForOperator(const string& name, const ai::strips::Goal& effect) {
    if (name == "MOVE") {
        if (effect.args.size() >= 2) {
            const string& dest = effect.args[1];
            if (dest == "Vault") return GoalType::REACH_VAULT;
            if (dest == "Exit") return GoalType::ESCAPE_TO_EXIT;
            if (dest == "A" || dest == "Alert") return GoalType::REACH_ALERT_ZONE;
        }
        return GoalType::MOVE_FLOOR_GOAL;
    }
    if (name == "ESCAPE") return GoalType::ESCAPE_TO_EXIT;
    return GoalType::NONE;
}

bool isSpecialRootGoal(const string& expr) {
    return expr == "steal(R,Vault) ^ escape(R)";
}

} // namespace

GoalStackPlanner::GoalStackPlanner(int robberId)
    : robberId(robberId), initialized(false), hasLastAstarTrace(false), heuristic(nullptr), rules(nullptr), vaultStolen(false) {}

void GoalStackPlanner::setHeuristicEngine(HeuristicEngine* h) {
    heuristic = h;
}

void GoalStackPlanner::setRuleEngine(RuleEngine* r) {
    rules = r;
}

string GoalStackPlanner::getPlanningDashboard() const {
    return goalStack.debugSummary();
}

void GoalStackPlanner::ensureInitialStack(const WorldState& world) {
    if (initialized && !goalStack.isEmpty()) return;
    goalStack.clear();
    GoalEntry escapeGoal;
    escapeGoal.goalExpression = "escape(R)";
    escapeGoal.isOperator = false;
    goalStack.push(escapeGoal);
    GoalEntry stealGoal;
    stealGoal.goalExpression = "steal(R,Vault)";
    stealGoal.isOperator = false;
    goalStack.push(stealGoal);
    initialized = true;
    vaultStolen = world.vaultStolen;
}

bool GoalStackPlanner::isGoalExpressionRoot(const string& expr) const {
    return expr == "steal(R,Vault) ^ escape(R)";
}

bool GoalStackPlanner::isGoalSatisfied(const GoalEntry& entry, const WorldState& world) const {
    ai::strips::Goal goal = ai::strips::Goal::parse(entry.goalExpression);
    if (goal.predicate == "steal") {
        return world.robberHasVault;
    }
    if (goal.predicate == "escape") {
        return world.robberPos == world.exitPos && (world.robberHasVault || world.vaultStolen);
    }
    if (goal.predicate == "clear") {
        if (!world.grid) return false;
        for (const auto& police : world.policePositions) {
            if (police == world.vaultPos) return false;
        }
        return true;
    }
    if (goal.predicate == "on" && goal.args.size() >= 2) {
        const string& target = goal.args[1];
        if (target == "Vault") return world.robberPos == world.vaultPos;
        if (target == "Exit") return world.robberPos == world.exitPos;
        if (target == "A" || target == "Alert") return world.grid && world.grid->getCell(world.robberPos) == CellType::ALERT_ZONE;
        if (target == "US") return world.grid && world.grid->getCell(world.robberPos) == CellType::STAIRS;
        if (target == "DS") return world.grid && world.grid->getCell(world.robberPos) == CellType::ELEVATOR;
    }
    if (goal.predicate == "samefloor" && goal.args.size() >= 2) {
        const string& dest = goal.args[1];
        int currentFloor = world.robberPos.z;
        if (!world.grid) return false;
        if (dest == "US") {
            for (int y = 0; y < world.grid->getHeight(); ++y) {
                for (int x = 0; x < world.grid->getWidth(); ++x) {
                    Position p(x, y, currentFloor);
                    if (world.grid->getCell(p) == CellType::STAIRS) return true;
                }
            }
            return false;
        }
        if (dest == "DS") {
            for (int y = 0; y < world.grid->getHeight(); ++y) {
                for (int x = 0; x < world.grid->getWidth(); ++x) {
                    Position p(x, y, currentFloor);
                    if (world.grid->getCell(p) == CellType::ELEVATOR) return true;
                }
            }
            return false;
        }
        Position destination;
        if (dest == "Vault") destination = world.vaultPos;
        else if (dest == "Exit") destination = world.exitPos;
        else if (dest == "A") {
            if (!world.grid) return false;
            auto alerts = world.grid->getAlertZones();
            if (alerts.empty()) return false;
            destination = alerts.front();
        } else {
            return false;
        }
        return currentFloor == destination.z;
    }
    if (isGoalExpressionRoot(entry.goalExpression)) {
        GoalEntry stealGoal;
        stealGoal.goalExpression = "steal(R,Vault)";
        stealGoal.isOperator = false;

        GoalEntry escapeGoal;
        escapeGoal.goalExpression = "escape(R)";
        escapeGoal.isOperator = false;

        return isGoalSatisfied(stealGoal, world) && isGoalSatisfied(escapeGoal, world);
    }
    return false;
}

bool GoalStackPlanner::decomposeGoal(const GoalEntry& top, WorldState& world) {
    ai::strips::Goal goal = ai::strips::Goal::parse(top.goalExpression);
    std::vector<ai::strips::Operator> operators = ai::strips::OperatorRegistry::instance().getOperatorsForGoal(goal, world);
    if (operators.empty()) {
        return false;
    }
    ai::strips::Operator op = operators.front();

    std::string opExpression;
    for (size_t i = 0; i < op.effect.args.size(); ++i) {
        if (i > 0) opExpression += ",";
        opExpression += op.effect.args[i];
    }
    opExpression = op.name + "(" + opExpression + ")";
    if (hasPendingGoal(opExpression)) {
        return false;
    }

    pushOperator(op);
    pushPreconditions(op, world);
    return true;
}

bool GoalStackPlanner::hasPendingGoal(const string& expr) const {
    for (const auto& entry : goalStack.getStack()) {
        if (entry.goalExpression == expr &&
            entry.status != GoalEntry::COMPLETED &&
            entry.status != GoalEntry::CANCELLED) {
            return true;
        }
    }
    return false;
}

bool GoalStackPlanner::policeOnVault(const WorldState& world) const {
    for (const auto& police : world.policePositions) {
        if (police == world.vaultPos) {
            return true;
        }
    }
    return false;
}

void GoalStackPlanner::pushOperator(const ai::strips::Operator& op) {
    GoalEntry entry;
    // Build a top-level operator expression using the operator's effect args
    // e.g. MOVE(R,US) instead of MOVE(on(R,US))
    std::string argText;
    for (size_t i = 0; i < op.effect.args.size(); ++i) {
        if (i > 0) argText += ",";
        argText += op.effect.args[i];
    }
    entry.goalExpression = op.name + "(" + argText + ")";
    entry.isOperator = true;
    entry.operatorName = op.name;
    for (const auto& pre : op.preconditions) {
        entry.preconditions.push_back(pre.toString());
    }
    for (const auto& arg : op.effect.args) {
        entry.effects.push_back(arg);
    }
    entry.operatorData = std::make_shared<ai::strips::Operator>(op);
    goalStack.push(entry);
}

void GoalStackPlanner::pushPreconditions(const ai::strips::Operator& op, WorldState& world) {
    for (auto it = op.preconditions.rbegin(); it != op.preconditions.rend(); ++it) {
        const string preExpr = it->toString();
        if (hasPendingGoal(preExpr)) continue;
        GoalEntry pre;
        pre.goalExpression = preExpr;
        pre.isOperator = false;
        if (isGoalSatisfied(pre, world)) {
            pre.status = GoalEntry::COMPLETED;
        }
        goalStack.push(pre);
    }
}

Position GoalStackPlanner::executeMove(const ai::strips::Operator& op, WorldState& world) {
    if (!world.grid) return world.robberPos;
    const ai::strips::Goal& effect = op.effect;
    if (effect.args.size() < 2) return world.robberPos;
    Position target = world.robberPos;
    const string& destination = effect.args[1];
    if (destination == "Vault") target = world.vaultPos;
    else if (destination == "Exit") target = world.exitPos;
    else if (destination == "A") target = world.alertPos;
    else if (destination == "US") target = ai::strips::OperatorRegistry::instance().findNearestCellOfType(*world.grid, world.robberPos, CellType::STAIRS);
    else if (destination == "DS") target = ai::strips::OperatorRegistry::instance().findNearestCellOfType(*world.grid, world.robberPos, CellType::ELEVATOR);

    world.activeTargetPos = target;
    world.activeGoalType = goalTypeForOperator(op.name, effect);

    vector<Position> path;
    if (heuristic && rules) {
        path = AStar3D::findPathWithHeuristic(world.robberPos, target, *world.grid, *heuristic, world,
                                             *rules,
                                             world.activeGoalType,
                                             world.policePositions,
                                             true,
                                             false,
                                             world.stepsTaken);
    } else {
        path = AStar3D::findPath(world.robberPos, target, *world.grid, world.activeGoalType, false, world.stepsTaken);
    }

    lastAstarTrace = AStar3D::getLastSearchTrace();
    hasLastAstarTrace = true;

    if (path.size() > 1) {
        world.robberPos = path[1];
    }
    return world.robberPos;
}

bool GoalStackPlanner::executeTop(WorldState& world) {
    if (goalStack.isEmpty()) return false;
    GoalEntry top = goalStack.peek();
    if (!top.isOperator) {
        if (isGoalSatisfied(top, world)) {
            goalStack.markComplete();
            goalStack.finalizeCompleted();
            return true;
        }
        return decomposeGoal(top, world);
    }
    if (!top.operatorData) {
        goalStack.cancel(top.goalExpression);
        goalStack.finalizeCompleted();
        return false;
    }
    const ai::strips::Operator& op = *top.operatorData;
    if (!op.checkPreconditions(world)) {
        pushPreconditions(op, world);
        return true;
    }
    if (op.name == "MOVE") {
        Position previous = world.robberPos;
        const ai::strips::Goal& effect = op.effect;
        Position target = previous;
        if (effect.args.size() >= 2) {
            const string& destination = effect.args[1];
            if (destination == "Vault") target = world.vaultPos;
            else if (destination == "Exit") target = world.exitPos;
            else if (destination == "A") target = world.alertPos;
            else if (destination == "US") target = ai::strips::OperatorRegistry::instance().findNearestCellOfType(*world.grid, world.robberPos, CellType::STAIRS);
            else if (destination == "DS") target = ai::strips::OperatorRegistry::instance().findNearestCellOfType(*world.grid, world.robberPos, CellType::ELEVATOR);
        }

        Position after = executeMove(op, world);
        if (after == target) {
            goalStack.markComplete();
            goalStack.finalizeCompleted();
            return true;
        }
        if (after != previous) {
            goalStack.markPerforming();
            return true;
        }
        return false;
    }
    op.applyEffects(world);
    goalStack.markComplete();
    goalStack.finalizeCompleted();
    return true;
}

Position GoalStackPlanner::runTurn(WorldState& world) {
    ensureInitialStack(world);

    // If police are on vault, re-push clear(Vault) even if previously completed
    // This handles environment changes where police step onto the vault
    if (policeOnVault(world)) {
        // Remove clear(Vault) from completed goals if present (environment changed)
        goalStack.removeGoalFromCompleted("clear(Vault)");

        // Ensure a fresh clear(Vault) goal is on top when police stand on the vault.
        bool topIsActiveClear = false;
        if (!goalStack.isEmpty()) {
            const GoalEntry& top = goalStack.peek();
            topIsActiveClear = (top.goalExpression == "clear(Vault)") &&
                               top.status != GoalEntry::COMPLETED &&
                               top.status != GoalEntry::CANCELLED;
        }
        if (!topIsActiveClear) {
            GoalEntry clearGoal;
            clearGoal.goalExpression = "clear(Vault)";
            clearGoal.isOperator = false;
            goalStack.push(clearGoal);
        }
    }

    goalStack.finalizeCompleted();

    for (int iteration = 0; iteration < 16 && !goalStack.isEmpty(); ++iteration) {
        GoalEntry top = goalStack.peek();
        if (top.status == GoalEntry::COMPLETED || top.status == GoalEntry::CANCELLED) {
            goalStack.finalizeCompleted();
            continue;
        }
        if (top.status == GoalEntry::PERFORMING && top.operatorData) {
            executeTop(world);
            break;
        }
        if (!executeTop(world)) {
            break;
        }
        if (!goalStack.isEmpty() && goalStack.peek().status == GoalEntry::PERFORMING) {
            break;
        }
    }
    return world.robberPos;
}

} // namespace planner
} // namespace ai
