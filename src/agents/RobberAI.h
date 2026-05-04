#ifndef ROBBER_AI_H
#define ROBBER_AI_H

#include "Agent.h"
#include "../ai/AStar3D.h"
#include "../ai/HeuristicEngine.h"
#include "../rules/RuleEngine.h"
#include "../planning/RobberAIPlanner.h"
#include "../planning/PlannerLog.h"
#include "../planning/PlannerVisualization.h"

using namespace std;

/**
 * RobberAI - Enhanced with Goal Stack Planning
 * 
 * Uses RobberAIPlanner for explainable decision-making:
 * - Main goal: escape(robber)
 * - Goal decomposition: vault → escape path
 * - Multi-objective heuristic for safe pathfinding
 * - Full logging of every decision
 */
enum class RobberState {
    HUNTING_VAULT,
    ESCAPING,
    EVASION,
    CORNERED
};

class RobberAI : public Agent {
private:
    Position targetGoal;
    RobberState state;
    bool hasVault;
    HeuristicEngine* heuristic;
    RuleEngine* rules;
    int turnsWaiting;
    Position lastPos;
    
    // NEW: Goal Stack Planning System
    unique_ptr<RobberAIPlanner> planner;
    PlannerLog planLog;
    string planningDashboard;
    int robberId;

public:
    RobberAI(const Position& startPos, int id, HeuristicEngine* h = nullptr);
    
    // Strategic functions (preserved for compatibility)
    void updateState(const Grid3D& grid, const vector<Position>& policePositions);
    Position getNextMove(const Grid3D& grid, const vector<Position>& policePositions) override;
    Position computeSafePath(const Grid3D& grid, 
                           const vector<Position>& policePositions);
    float evaluateDanger(const Position& p, const vector<Position>& policePositions) const;
    
    // NEW: Planning system accessors
    RobberAIPlanner* getPlanner() { return planner.get(); }
    const PlannerLog& getPlanLog() const { return planLog; }
    const string& getPlanningDashboard() const { return planningDashboard; }
    void setHasVault(bool value) { hasVault = value; }
    
    // Getters/Setters
    RobberState getState() const { return state; }
    bool hasReachedVault(const Grid3D& grid) const;
    bool hasReachedExit(const Grid3D& grid) const;
    bool hasCollectedVault() const { return hasVault; }
    void setHeuristic(HeuristicEngine* h) { heuristic = h; }
    void setRuleEngine(RuleEngine* r) { rules = r; if (planner) planner->setRuleEngine(r); }
};

#endif
