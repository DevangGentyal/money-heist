#include "RuleEngine.h"

RuleEngine::RuleEngine(int difficulty) 
    : baseDifficulty(difficulty), currentState(DifficultyState::NORMAL) {
    context.cctvTriggered = false;
    context.alertTriggered = false;
    context.turnsInAlert = 0;
    context.policeSpeed = 1.0f;
    context.predictionDepth = 8 + difficulty * 2;
    context.playerDetected = false;
    context.turnsAtLarge = 0;
    
    if (difficulty == 1) {
        currentState = DifficultyState::EASY;
        context.policeSpeed = 0.7f;
        context.predictionDepth = 6;
    } else if (difficulty == 2) {
        currentState = DifficultyState::NORMAL;
        context.policeSpeed = 1.0f;
        context.predictionDepth = 10;
    } else if (difficulty == 3) {
        currentState = DifficultyState::HARD;
        context.policeSpeed = 1.3f;
        context.predictionDepth = 12;
    }
}

void RuleEngine::updateContext(const GameContext& ctx) {
    context = ctx;
    evaluateDifficulty();
}

void RuleEngine::evaluateDifficulty() {
    if (context.alertTriggered) {
        context.turnsInAlert++;
        if (context.turnsInAlert > 5) {
            currentState = DifficultyState::EXTREME;
        } else {
            currentState = DifficultyState::HARD;
        }
    } else if (context.cctvTriggered) {
        currentState = DifficultyState::HARD;
        context.turnsInAlert = max(0, context.turnsInAlert - 1);
    } else {
        if (context.turnsInAlert > 0) {
            context.turnsInAlert--;
        }
        if (baseDifficulty == 1) {
            currentState = DifficultyState::EASY;
        } else if (baseDifficulty == 2) {
            currentState = DifficultyState::NORMAL;
        } else {
            currentState = DifficultyState::HARD;
        }
    }
}

float RuleEngine::getPoliceSpeedMultiplier() const {
    switch (currentState) {
        case DifficultyState::EASY:
            return 0.7f;
        case DifficultyState::NORMAL:
            return 1.0f;
        case DifficultyState::HARD:
            return 1.3f;
        case DifficultyState::EXTREME:
            return 1.6f;
    }
    return 1.0f;
}

int RuleEngine::getPredictionDepth() const {
    switch (currentState) {
        case DifficultyState::EASY:
            return 5;
        case DifficultyState::NORMAL:
            return 10;
        case DifficultyState::HARD:
            return 15;
        case DifficultyState::EXTREME:
            return 20;
    }
    return 10;
}

float RuleEngine::getAlertZoneSize() const {
    if (context.alertTriggered) {
        return 8.0f;
    } else if (context.cctvTriggered) {
        return 5.0f;
    }
    return 0.0f;
}

void RuleEngine::triggerCCTV(const Position& /*pos*/) {
    context.cctvTriggered = true;
    context.turnsInAlert = max(context.turnsInAlert, 2);
}

void RuleEngine::triggerAlert(const Position& /*pos*/) {
    context.alertTriggered = true;
    context.turnsInAlert = 5;
}

void RuleEngine::resetAlert() {
    context.alertTriggered = false;
    context.turnsInAlert = 0;
}

void RuleEngine::onPlayerDetected() {
    context.playerDetected = true;
    context.turnsAtLarge = 0;
}
