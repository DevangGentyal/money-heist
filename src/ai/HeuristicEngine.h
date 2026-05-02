#ifndef HEURISTIC_ENGINE_H
#define HEURISTIC_ENGINE_H

#include "../grid/Grid3D.h"
#include "../planning/STRIPSOperators.h"
#include "../rules/RuleEngine.h"
#include <vector>

using namespace std;

struct Node;

class HeuristicEngine {
private:
    const Grid3D& grid;

public:
    HeuristicEngine(const Grid3D& g);

    float compute(const Node& n, GoalType g, WorldState& w, RuleEngine& rules);
    void adjustForDifficulty(int) {}

    float scoreDistance(const Position& current, const Position& goal) const;
    float scorePoliceProximity(const Position& current, const vector<Position>& police) const;
    float scoreCCTVRisk(const Position& current) const;
    float scoreAlertZoneRisk(const Position& current) const;
    float scoreVerticalCost(const Position& current, const Position& goal) const;
    float scoreTransitionControl(const Position& current, const Position& goal) const;
    float scoreTransitionExposure(const Position& current, const vector<Position>& police) const;
    float scoreFloorDifference(const Position& current, const Position& goal) const;
    float scoreTransitionBonus(const Position& current) const;
    float scoreVaultExposurePenalty(const Position& current, const Position& vaultPos) const;
    float scoreAlertZoneBonus(const Position& current) const;
    float scorePoliceDanger(const Position& current, const vector<Position>& police) const;
};

#endif
