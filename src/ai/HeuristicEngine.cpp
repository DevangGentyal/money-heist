#include "HeuristicEngine.h"

#include "AStar3D.h"
#include <algorithm>
#include <cmath>

namespace {
int nearestTransitionDistance(const Grid3D& grid, const Position& from, CellType targetType) {
    int best = 1000000;
    for (int y = 1; y < grid.getHeight() - 1; ++y) {
        for (int x = 1; x < grid.getWidth() - 1; ++x) {
            Position p(x, y, from.z);
            if (grid.getCell(p) == targetType) {
                best = min(best, grid.manhattanDistance(from, p));
            }
        }
    }
    return best == 1000000 ? 0 : best;
}
}

HeuristicEngine::HeuristicEngine(const Grid3D& g) : grid(g) {}

float HeuristicEngine::compute(const Node& n, GoalType g, WorldState& w, RuleEngine& rules) {
    HeuristicWeights weights = rules.getWeights(g);
    const Position current = n.pos;

    switch (g) {
        case GoalType::PROTECT_VAULT:
            return weights.w1 * scoreDistance(current, w.vaultPos) +
                   weights.w2 * (1.0f / (scoreDistance(current, w.robberPos) + 1.0f)) +
                   weights.w3 * (-scoreCCTVRisk(current)) +
                   weights.w4 * scoreAlertZoneRisk(current);
        case GoalType::CHASE_ROBBER:
            return weights.w1 * scoreDistance(current, w.robberPos) +
                   weights.w2 * scoreFloorDifference(current, w.robberPos) +
                   weights.w3 * scoreTransitionBonus(current);
        case GoalType::RESPOND_ALERT:
            return weights.w1 * scoreDistance(current, w.alertPos) +
                   weights.w2 * scoreVaultExposurePenalty(current, w.vaultPos);
        case GoalType::REACH_VAULT:
            return weights.w1 * scoreDistance(current, w.vaultPos) +
                   weights.w2 * scorePoliceDanger(current, w.policePositions) +
                   weights.w3 * scoreAlertZoneBonus(current);
        case GoalType::ESCAPE_TO_EXIT:
            return weights.w1 * scoreDistance(current, w.exitPos) +
                   weights.w2 * scorePoliceDanger(current, w.policePositions) +
                   weights.w3 * scoreTransitionBonus(current);
        case GoalType::REACH_ALERT_ZONE:
            return weights.w1 * scoreDistance(current, w.alertPos) +
                   weights.w2 * scorePoliceDanger(current, w.policePositions);
        case GoalType::MOVE_FLOOR_GOAL:
            return weights.w1 * scoreVerticalCost(current, w.activeTargetPos) +
                   weights.w2 * scoreTransitionControl(current, w.activeTargetPos);
        default:
            return scoreDistance(current, w.activeTargetPos);
    }
}

float HeuristicEngine::scoreDistance(const Position& current, const Position& goal) const {
    return grid.manhattanDistance(current, goal);
}

float HeuristicEngine::scorePoliceProximity(const Position& current, const vector<Position>& police) const {
    if (police.empty()) return 0.0f;
    float minDistance = 1000000.0f;
    for (const auto& p : police) {
        minDistance = min(minDistance, static_cast<float>(grid.manhattanDistance(current, p)));
    }
    if (minDistance < 3) return 10.0f + (3 - minDistance) * 5.0f;
    if (minDistance < 6) return 5.0f;
    return 0.0f;
}

float HeuristicEngine::scoreCCTVRisk(const Position& current) const {
    for (const auto& cctvPos : grid.getCCTVPositions()) {
        int dist = grid.manhattanDistance(current, cctvPos);
        if (dist <= 3) return 10.0f - dist * 2.0f;
    }
    return 0.0f;
}

float HeuristicEngine::scoreAlertZoneRisk(const Position& current) const {
    for (const auto& alertPos : grid.getAlertZones()) {
        int dist = grid.manhattanDistance(current, alertPos);
        if (dist <= 2) return 5.0f - dist * 1.5f;
    }
    return 0.0f;
}

float HeuristicEngine::scoreVerticalCost(const Position& current, const Position& goal) const {
    return grid.verticalCost(current, goal);
}

float HeuristicEngine::scoreTransitionControl(const Position& current, const Position& goal) const {
    if (current.z == goal.z) return 0.0f;
    return static_cast<float>(nearestTransitionDistance(grid, current, CellType::STAIRS));
}

float HeuristicEngine::scoreTransitionExposure(const Position& current, const vector<Position>& police) const {
    CellType tile = grid.getCell(current);
    if (tile != CellType::STAIRS) return 0.0f;
    float minDistance = 1000000.0f;
    for (const auto& policePos : police) {
        minDistance = min(minDistance, static_cast<float>(grid.manhattanDistance(current, policePos)));
    }
    if (minDistance <= 1.0f) return 18.0f;
    if (minDistance <= 2.0f) return 10.0f;
    if (minDistance <= 3.0f) return 4.0f;
    return 0.0f;
}

float HeuristicEngine::scoreFloorDifference(const Position& current, const Position& goal) const {
    return static_cast<float>(abs(current.z - goal.z) * 3);
}

float HeuristicEngine::scoreTransitionBonus(const Position& current) const {
    CellType type = grid.getCell(current);
    if (type == CellType::STAIRS) {
        return -2.0f;
    }
    return 0.0f;
}

float HeuristicEngine::scoreVaultExposurePenalty(const Position& current, const Position& vaultPos) const {
    return grid.manhattanDistance(current, vaultPos) <= 2 ? 3.0f : 0.0f;
}

float HeuristicEngine::scoreAlertZoneBonus(const Position& current) const {
    return grid.isAlertZone(current) ? -1.0f : 0.0f;
}

float HeuristicEngine::scorePoliceDanger(const Position& current, const vector<Position>& police) const {
    return scorePoliceProximity(current, police);
}
