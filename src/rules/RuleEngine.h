#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#include "../grid/Grid3D.h"
#include <vector>

using namespace std;

enum class DifficultyState {
    EASY,
    NORMAL,
    HARD,
    EXTREME
};

struct GameContext {
    bool cctvTriggered;
    bool alertTriggered;
    int turnsInAlert;
    float policeSpeed;
    int predictionDepth;
    bool playerDetected;
    int turnsAtLarge;
};

class RuleEngine {
private:
    int baseDifficulty;
    GameContext context;
    DifficultyState currentState;
    
public:
    RuleEngine(int difficulty = 1);
    
    // Update context based on game events
    void updateContext(const GameContext& ctx);
    
    // Evaluate difficulty adjustments
    void evaluateDifficulty();
    
    // Get current difficulty modifiers
    float getPoliceSpeedMultiplier() const;
    int getPredictionDepth() const;
    float getAlertZoneSize() const;
    
    // Event triggers
    void triggerCCTV(const Position& pos);
    void triggerAlert(const Position& pos);
    void resetAlert();
    void onPlayerDetected();
    
    // Getters
    DifficultyState getCurrentState() const { return currentState; }
    bool isCCTVActive() const { return context.cctvTriggered; }
    bool isAlertActive() const { return context.alertTriggered; }
};

#endif
