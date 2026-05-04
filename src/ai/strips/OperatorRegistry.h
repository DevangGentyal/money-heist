#pragma once

#include <vector>
#include <string>
#include "Operator.h"
#include "../../planning/STRIPSOperators.h"

namespace ai {
namespace strips {

class OperatorRegistry {
public:
    static const OperatorRegistry& instance();
    std::vector<Operator> getOperatorsForGoal(const Goal& goal, const WorldState& world) const;

private:
    OperatorRegistry();
    Operator makeStealOperator() const;
    Operator makeTriggerOperator() const;
    Operator makeMoveOperator(const Goal& goal, const WorldState& world) const;
    Operator makeClimbOperator(const Goal& goal, const WorldState& world) const;
    Operator makeDescendOperator(const Goal& goal, const WorldState& world) const;
    Operator makeEscapeOperator() const;

public:
    // Exposed helpers for symbol translation and nearest-target searches
    Position findNearestCellOfType(const Grid3D& grid,
                                   const Position& origin,
                                   CellType type) const;
    Position findNearestAlertZone(const WorldState& world,
                                  const Position& origin) const;

private:
    bool isGoalClear(const Goal& goal, const WorldState& world) const;
    bool isSameFloorTargetAvailable(const Goal& goal, const WorldState& world) const;
};

} // namespace strips
} // namespace ai
