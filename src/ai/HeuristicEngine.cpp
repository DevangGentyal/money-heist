#include "HeuristicEngine.h"
#include <cmath>
#include <algorithm>

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

    if (best == 1000000) {
        return 0;
    }
    return best;
}
} // namespace

HeuristicEngine::HeuristicEngine(const Grid3D& g) : grid(g) {}

float HeuristicEngine::calculateHeuristic(const Position& current, 
                                         const Position& goal, 
                                         const vector<Position>& policePositions,
                                         bool isRobberPerspective,
                                         bool canTransitionFloors) {
    float score = 0.0f;
    
    // If police can't transition floors and goal is on different floor, penalize heavily
    if (!isRobberPerspective && !canTransitionFloors && current.z != goal.z) {
        // Ignore vertical distance - only score horizontal
        int horizontalDist = abs(current.x - goal.x) + abs(current.y - goal.y);
        score += weights.w_distance * horizontalDist;
    } else {
        // Distance component
        score += weights.w_distance * scoreDistance(current, goal);
    }
    
    // Police proximity (more important for robber)
    if (isRobberPerspective) {
        score += weights.w_police_proximity * scorePoliceProximity(current, policePositions);
    }
    
    // CCTV avoidance
    score += weights.w_cctv_penalty * scoreCCTVRisk(current);
    
    // Alert zone avoidance
    score += weights.w_alert_zone_penalty * scoreAlertZoneRisk(current);
    
    // Vertical movement cost - but penalize stairs/elevators when no transition allowed
    if (!canTransitionFloors && !isRobberPerspective) {
        CellType cellType = grid.getCell(current);
        if (cellType == CellType::STAIRS || cellType == CellType::ELEVATOR) {
            score += 50.0f; // Heavy penalty for standing on stairs when can't transition
        }
    } else {
        // Normal vertical movement cost
        score += weights.w_vertical_cost * scoreVerticalCost(current, goal);
    }

    if (isRobberPerspective) {
        score += weights.w_transition_exposure * scoreTransitionExposure(current, policePositions);
    } else {
        score += weights.w_transition_control * scoreTransitionControl(current, goal, canTransitionFloors);
    }
    
    return score;
}

float HeuristicEngine::scoreDistance(const Position& current, const Position& goal) {
    return grid.manhattanDistance(current, goal);
}

float HeuristicEngine::scorePoliceProximity(const Position& current, 
                                          const vector<Position>& police) {
    if (police.empty()) return 0.0f;
    
    float minDistance = 1000000.0f;
    for (const auto& p : police) {
        int dist = grid.manhattanDistance(current, p);
        minDistance = min(minDistance, (float)dist);
    }
    
    // Closer to police = higher penalty (reciprocal)
    if (minDistance < 3) return 10.0f + (3 - minDistance) * 5.0f;
    if (minDistance < 6) return 5.0f;
    return 0.0f;
}

float HeuristicEngine::scoreCCTVRisk(const Position& current) {
    for (const auto& cctvPos : grid.getCCTVPositions()) {
        int dist = grid.manhattanDistance(current, cctvPos);
        if (dist <= 3) {
            return 10.0f - dist * 2.0f;
        }
    }
    return 0.0f;
}

float HeuristicEngine::scoreAlertZoneRisk(const Position& current) {
    for (const auto& alertPos : grid.getAlertZones()) {
        int dist = grid.manhattanDistance(current, alertPos);
        if (dist <= 2) {
            return 5.0f - dist * 1.5f;
        }
    }
    return 0.0f;
}

float HeuristicEngine::scoreVerticalCost(const Position& current, const Position& goal) {
    return grid.verticalCost(current, goal);
}

float HeuristicEngine::scoreTransitionControl(const Position& current,
                                             const Position& goal,
                                             bool canTransitionFloors) {
    if (current.z == goal.z) {
        return 0.0f;
    }

    CellType desired = goal.z > current.z ? CellType::STAIRS : CellType::ELEVATOR;
    int dist = nearestTransitionDistance(grid, current, desired);
    float controlScore = static_cast<float>(dist);

    // When police cannot transition yet, force stronger preference for chokepoints.
    if (!canTransitionFloors) {
        controlScore *= 1.8f;
    }

    return controlScore;
}

float HeuristicEngine::scoreTransitionExposure(const Position& current,
                                              const vector<Position>& police) {
    CellType tile = grid.getCell(current);
    if (tile != CellType::STAIRS && tile != CellType::ELEVATOR) {
        return 0.0f;
    }

    float minDistance = 1000000.0f;
    for (const auto& policePos : police) {
        int dist = grid.manhattanDistance(current, policePos);
        minDistance = min(minDistance, static_cast<float>(dist));
    }

    if (minDistance <= 1.0f) return 18.0f;
    if (minDistance <= 2.0f) return 10.0f;
    if (minDistance <= 3.0f) return 4.0f;
    return 0.0f;
}

void HeuristicEngine::adjustForDifficulty(int difficulty) {
    if (difficulty == 1) {
        weights.w_distance = 1.0f;
        weights.w_police_proximity = 0.2f;
        weights.w_cctv_penalty = 0.5f;
        weights.w_transition_control = 0.5f; // Lowered
        weights.w_transition_exposure = 1.0f;
    } else if (difficulty == 2) {
        weights.w_distance = 1.5f; // Boosted
        weights.w_police_proximity = 0.5f;
        weights.w_cctv_penalty = 1.5f;
        weights.w_transition_control = 1.0f; // Lowered
        weights.w_transition_exposure = 1.8f;
    } else if (difficulty == 3) {
        weights.w_distance = 2.5f; // High boost for aggressive pursuit
        weights.w_police_proximity = 1.0f;
        weights.w_cctv_penalty = 2.5f;
        weights.w_transition_control = 1.5f; // Lowered
        weights.w_transition_exposure = 2.6f;
    }
}
