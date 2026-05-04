#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../../planning/GoalStack.h"
#include "../../planning/STRIPSOperators.h"
#include "../HeuristicEngine.h"
#include "../../rules/RuleEngine.h"
#include "../AStar3D.h"
#include "../strips/OperatorRegistry.h"

namespace ai {
namespace planner {

class GoalStackPlanner {
public:
    explicit GoalStackPlanner(int robberId = 0);
    virtual ~GoalStackPlanner() = default;

    void setHeuristicEngine(HeuristicEngine* h);
    void setRuleEngine(RuleEngine* r);

    Position runTurn(WorldState& world);
    const GoalStack& getGoalStack() const { return goalStack; }
    string getPlanningDashboard() const;
    const SearchTrace& getLastAstarTrace() const { return lastAstarTrace; }
    bool hasAstarTrace() const { return hasLastAstarTrace; }

private:
    GoalStack goalStack;
    int robberId;
    bool initialized;
    bool hasLastAstarTrace;
    SearchTrace lastAstarTrace;
    HeuristicEngine* heuristic;
    RuleEngine* rules;
    bool vaultStolen;

    void ensureInitialStack(const WorldState& world);
    bool executeTop(WorldState& world);
    bool decomposeGoal(const GoalEntry& top, WorldState& world);
    bool isGoalSatisfied(const GoalEntry& entry, const WorldState& world) const;
    void pushOperator(const ai::strips::Operator& op);
    void pushPreconditions(const ai::strips::Operator& op, WorldState& world);
    Position executeMove(const ai::strips::Operator& op, WorldState& world);
    bool isGoalExpressionRoot(const string& expr) const;
    bool hasPendingGoal(const string& expr) const;
    bool policeOnVault(const WorldState& world) const;
};

} // namespace planner
} // namespace ai
