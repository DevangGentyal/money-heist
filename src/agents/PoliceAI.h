#ifndef POLICE_AI_H
#define POLICE_AI_H

#include "Agent.h"
#include "../ai/AStar3D.h"
#include "../ai/PredictionEngine.h"
#include "../ai/HeuristicEngine.h"
#include "../rules/RuleEngine.h"
#include "../planning/PoliceAIPlanner.h"
#include "../planning/PlannerLog.h"
#include "../planning/PlannerVisualization.h"

using namespace std;

/**
 * PoliceAI - Enhanced with Goal Stack Planning
 * 
 * Uses PoliceAIPlanner for explainable decision-making:
 * - Main goal: catch(police, robber)
 * - Goal decomposition through operators
 * - A* pathfinding for execution
 * - Full logging of every decision
 */
enum class PoliceState {
    PATROL,
    ALERT,
    CHASE,
    INTERCEPT
};

class PoliceAI : public Agent {
private:
    int policeIndex;           // 0-based index for multi-police tracking
    PoliceState state;
    Position lastKnownTargetPos;
    Position interceptionTarget;
    PredictionEngine* predictor;
    HeuristicEngine* heuristic;
    RuleEngine* rules;
    bool vaultCollected;
    
    // Goal Stack Planning System
    unique_ptr<PoliceAIPlanner> planner;
    PlannerLog planLog;
    bool hardMode;
    string planningDashboard;
    
public:
    PoliceAI(const Position& startPos,
            int idx,
            PredictionEngine* pred = nullptr,
            HeuristicEngine* h = nullptr,
            RuleEngine* r = nullptr,
            bool hard = false);
    
    Position getNextMove(const Grid3D& grid,
                        const vector<Position>& otherAgents) override;
    
    // Strategic functions (preserved for compatibility)
    void updateState(const Grid3D& grid, const Position& targetPos, bool isDetected);
    Position computeInterceptPath(const Grid3D& grid, const Position& targetPos);
    Position predictTargetPosition(const Grid3D& grid, const Position& targetPos);
    
    // NEW: Planning system accessors
    PoliceAIPlanner* getPlanner() { return planner.get(); }
    const PlannerLog& getPlanLog() const { return planLog; }
    const string& getPlanningDashboard() const { return planningDashboard; }
    void setHardMode(bool hard) { hardMode = hard; }
    bool isHardMode() const { return hardMode; }
    
    // Getters/Setters
    PoliceState getState() const { return state; }
    void setPredictionEngine(PredictionEngine* p) { predictor = p; }
    void setHeuristic(HeuristicEngine* h) { heuristic = h; }
    void setRuleEngine(RuleEngine* r) { rules = r; }
    void setVaultCollected(bool collected) { vaultCollected = collected; }
    bool isVaultCollected() const { return vaultCollected; }
};

#endif
