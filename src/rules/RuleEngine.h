#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#include "../grid/Grid3D.h"
#include "../planning/STRIPSOperators.h"
#include <algorithm>
#include <vector>

using namespace std;

enum class DifficultyState {
    EASY,
    NORMAL,
    HARD,
    EXTREME
};

struct HeuristicWeights {
    float w1;
    float w2;
    float w3;
    float w4;
};

struct GameContext {
    bool cctvTriggered;
    bool alertTriggered;
    int turnsInAlert;
    float policeSpeed;
    int predictionDepth;
    bool playerDetected;
    int turnsAtLarge;
    bool vaultStolen;
    Position alertPos;

    GameContext()
        : cctvTriggered(false), alertTriggered(false), turnsInAlert(0), policeSpeed(1.0f),
          predictionDepth(10), playerDetected(false), turnsAtLarge(0), vaultStolen(false), alertPos() {}
};

class RuleEngine {
private:
    int baseDifficulty;
    GameContext context;
    DifficultyState currentState;

public:
    inline RuleEngine(int difficulty = 1)
        : baseDifficulty(difficulty), currentState(DifficultyState::NORMAL) {
        if (difficulty == 1) {
            currentState = DifficultyState::EASY;
            context.policeSpeed = 0.7f;
            context.predictionDepth = 6;
        } else if (difficulty == 2) {
            currentState = DifficultyState::NORMAL;
            context.policeSpeed = 1.0f;
            context.predictionDepth = 10;
        } else {
            currentState = DifficultyState::HARD;
            context.policeSpeed = 1.3f;
            context.predictionDepth = 12;
        }
    }

    inline void updateContext(const GameContext& ctx) {
        context = ctx;
        evaluateDifficulty();
    }

    inline void evaluateDifficulty() {
        if (context.alertTriggered) {
            context.turnsInAlert++;
            currentState = context.turnsInAlert > 5 ? DifficultyState::EXTREME : DifficultyState::HARD;
        } else if (context.cctvTriggered) {
            currentState = DifficultyState::HARD;
            context.turnsInAlert = max(0, context.turnsInAlert - 1);
        } else {
            if (baseDifficulty == 1) {
                currentState = DifficultyState::EASY;
            } else if (baseDifficulty == 2) {
                currentState = DifficultyState::NORMAL;
            } else {
                currentState = DifficultyState::HARD;
            }
        }
    }

    inline float getPoliceSpeedMultiplier() const {
        switch (currentState) {
            case DifficultyState::EASY: return 0.7f;
            case DifficultyState::NORMAL: return 1.0f;
            case DifficultyState::HARD: return 1.3f;
            case DifficultyState::EXTREME: return 1.6f;
        }
        return 1.0f;
    }

    inline int getPredictionDepth() const {
        switch (currentState) {
            case DifficultyState::EASY: return 6;
            case DifficultyState::NORMAL: return 10;
            case DifficultyState::HARD: return 15;
            case DifficultyState::EXTREME: return 20;
        }
        return 10;
    }

    inline float getAlertZoneSize() const {
        if (context.alertTriggered) {
            return 8.0f;
        }
        if (context.cctvTriggered) {
            return 5.0f;
        }
        return 0.0f;
    }

    inline bool isAlertActive() const { return context.alertTriggered; }
    inline bool isBoostActive() const { return context.turnsInAlert > 0 || context.alertTriggered; }
    inline bool isVaultStolen() const { return context.vaultStolen; }
    inline Position getAlertPos() const { return context.alertPos; }

    inline HeuristicWeights getWeights(GoalType g) const {
        switch (g) {
            case GoalType::PROTECT_VAULT: return {1.0f, 1.8f, 0.7f, 1.0f};
            case GoalType::CHASE_ROBBER: return {1.4f, 1.2f, 0.8f, 0.0f};
            case GoalType::RESPOND_ALERT: return {1.2f, 0.8f, 0.0f, 1.4f};
            case GoalType::REACH_VAULT: return {1.2f, 1.4f, 0.8f, 0.0f};
            case GoalType::ESCAPE_TO_EXIT: return {1.4f, 1.2f, 0.8f, 0.0f};
            case GoalType::REACH_ALERT_ZONE: return {1.0f, 1.6f, 0.0f, 0.0f};
            case GoalType::MOVE_FLOOR_GOAL: return {1.0f, 0.0f, 0.8f, 0.0f};
            default: return {1.0f, 1.0f, 0.5f, 0.5f};
        }
    }

    inline void triggerCCTV(const Position& /*pos*/) {
        context.cctvTriggered = true;
        context.turnsInAlert = max(context.turnsInAlert, 2);
    }

    inline void triggerAlert(const Position& pos) {
        context.alertTriggered = true;
        context.alertPos = pos;
        context.turnsInAlert = 5;
    }

    inline void notifyVaultStolen() { context.vaultStolen = true; }
    inline void notifyAlertTriggered(Cell pos) { triggerAlert(pos); }
    inline void resetAlert() { context.alertTriggered = false; context.turnsInAlert = 0; }
    inline void onPlayerDetected() { context.playerDetected = true; context.turnsAtLarge = 0; }

    DifficultyState getCurrentState() const { return currentState; }
    bool isCCTVActive() const { return context.cctvTriggered; }
};

#endif
