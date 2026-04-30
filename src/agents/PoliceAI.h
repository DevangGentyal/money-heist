#ifndef POLICE_AI_H
#define POLICE_AI_H

#include "Agent.h"
#include "../ai/AStar3D.h"
#include "../ai/PredictionEngine.h"
#include "../ai/HeuristicEngine.h"
#include "../rules/RuleEngine.h"

using namespace std;

enum class PoliceState {
    PATROL,
    ALERT,
    CHASE,
    INTERCEPT
};

class PoliceAI : public Agent {
private:
    PoliceState state;
    Position lastKnownTargetPos;
    Position interceptionTarget;
    PredictionEngine* predictor;
    HeuristicEngine* heuristic;
    RuleEngine* rules;
    bool vaultCollected;
    
public:
    PoliceAI(const Position& startPos,
            PredictionEngine* pred = nullptr,
            HeuristicEngine* h = nullptr,
            RuleEngine* r = nullptr);
    
    Position getNextMove(const Grid3D& grid,
                        const vector<Position>& otherAgents) override;
    
    // Strategic functions
    void updateState(const Grid3D& grid, const Position& targetPos, bool isDetected);
    Position computeInterceptPath(const Grid3D& grid, const Position& targetPos);
    Position predictTargetPosition(const Grid3D& grid, const Position& targetPos);
    
    // Getters/Setters
    PoliceState getState() const { return state; }
    void setPredictionEngine(PredictionEngine* p) { predictor = p; }
    void setHeuristic(HeuristicEngine* h) { heuristic = h; }
    void setRuleEngine(RuleEngine* r) { rules = r; }
    void setVaultCollected(bool collected) { vaultCollected = collected; }
    bool isVaultCollected() const { return vaultCollected; }
};

#endif
