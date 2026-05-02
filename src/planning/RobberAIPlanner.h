#ifndef ROBBER_AI_PLANNER_H
#define ROBBER_AI_PLANNER_H

#include "Planner.h"
#include "../ai/HeuristicEngine.h"
#include "STRIPSOperators.h"
#include "../ai/AStar3D.h"

using namespace std;

class RobberAIPlanner : public Planner {
private:
    HeuristicEngine* heuristic;
    RuleEngine* rules;
    Position vaultPos;
    Position exitPos;
    bool hasVault;
    bool initialized;
    // Last A* trace captured for debug snapshot
    SearchTrace lastAstarTrace;
    bool hasLastAstarTrace = false;

public:
    RobberAIPlanner();
    virtual ~RobberAIPlanner() {}

    void setHeuristicEngine(HeuristicEngine* h) { heuristic = h; }
    void setRuleEngine(RuleEngine* r) { rules = r; }
    void initializePlan(const WorldState& world);
    Position runTurn(WorldState& world);
    string getPlanningDashboard() const;
    // accessors for snapshot writer
    const SearchTrace& getLastAstarTrace() const { return lastAstarTrace; }
    bool hasAstarTrace() const { return hasLastAstarTrace; }

private:
    GoalEntry makeGoal(const string& expression, const vector<string>& preconditions = {}, const vector<string>& effects = {});
    GoalEntry makeOperator(const string& name, const vector<string>& preconditions, const vector<string>& effects);
    void ensureInitialStack(WorldState& world);
    void decomposeTop(WorldState& world);
    bool executeTop(WorldState& world);
    Position executeMove(const Position& target, WorldState& world, GoalType goalType);
    float evaluateDanger(const Grid3D& grid, const Position& pos, const Position& policePos) const;
};

#endif
