#include "OperatorRegistry.h"
#include <limits>
#include <sstream>

namespace ai {
namespace strips {

const OperatorRegistry& OperatorRegistry::instance() {
    static OperatorRegistry registry;
    return registry;
}

OperatorRegistry::OperatorRegistry() {}

static int floorOfSymbol(const std::string& symbol, const WorldState& world) {
    if (symbol == "Vault") return world.vaultPos.z;
    if (symbol == "Exit") return world.exitPos.z;
    if (symbol == "R") return world.robberPos.z;
    if (symbol == "A" || symbol == "Alert") {
        if (world.grid) {
            for (const auto& alertPos : world.grid->getAlertZones()) {
                return alertPos.z;
            }
        }
    }
    return world.robberPos.z;
}

static Position targetPositionForSymbol(const std::string& symbol, const WorldState& world, const OperatorRegistry& registry) {
    if (symbol == "Vault") return world.vaultPos;
    if (symbol == "Exit") return world.exitPos;
    if (symbol == "A" || symbol == "Alert") return registry.findNearestAlertZone(world, world.robberPos);
    if (symbol == "US") return registry.findNearestCellOfType(*world.grid, world.robberPos, CellType::STAIRS);
    if (symbol == "DS") return registry.findNearestCellOfType(*world.grid, world.robberPos, CellType::ELEVATOR);
    return world.robberPos;
}

std::vector<Operator> OperatorRegistry::getOperatorsForGoal(const Goal& goal, const WorldState& world) const {
    std::vector<Operator> ops;
    if (goal.predicate == "steal") {
        ops.push_back(makeStealOperator());
        return ops;
    }
    if (goal.predicate == "clear") {
        ops.push_back(makeTriggerOperator());
        return ops;
    }
    if (goal.predicate == "on") {
        Operator op = makeMoveOperator(goal, world);
        if (!op.name.empty()) ops.push_back(std::move(op));
        return ops;
    }
    if (goal.predicate == "samefloor") {
        if (!world.grid) return ops;
        if (goal.args.size() < 2) return ops;
        std::string dest = goal.args[1];
        int destFloor = floorOfSymbol(dest, world);
        int currentFloor = world.robberPos.z;
        if (destFloor == currentFloor) {
            return ops;
        }
        if (destFloor > currentFloor) {
            Operator op = makeClimbOperator(goal, world);
            if (!op.name.empty()) ops.push_back(std::move(op));
            return ops;
        }
        Operator op = makeDescendOperator(goal, world);
        if (!op.name.empty()) ops.push_back(std::move(op));
        return ops;
    }
    if (goal.predicate == "escape") {
        if (!(world.robberHasVault || world.vaultStolen)) {
            return ops;
        }
        ops.push_back(makeEscapeOperator());
        return ops;
    }
    return ops;
}

Operator OperatorRegistry::makeStealOperator() const {
    Operator op;
    op.name = "STEAL";
    op.effect = Goal("steal", {"R", "Vault"});
    op.preconditions = {Goal("clear", {"Vault"}), Goal("on", {"R", "Vault"})};
    op.checkPreconditions = [](const WorldState& w) {
        return w.robberPos == w.vaultPos && !w.vaultStolen;
    };
    op.applyEffects = [](WorldState& w) {
        w.vaultStolen = true;
        w.robberHasVault = true;
    };
    return op;
}

Operator OperatorRegistry::makeTriggerOperator() const {
    Operator op;
    op.name = "TRIGGER";
    op.effect = Goal("clear", {"Vault"});
    op.preconditions = {Goal("on", {"R", "Alert"})};
    op.checkPreconditions = [](const WorldState& w) {
        if (!w.grid) return false;
        CellType current = w.grid->getCell(w.robberPos);
        return current == CellType::ALERT_ZONE;
    };
    op.applyEffects = [](WorldState& w) {
        if (w.grid && w.grid->isAlertZone(w.robberPos)) {
            w.alertTriggered = true;
            w.alertPos = w.robberPos;
        }
    };
    return op;
}

Operator OperatorRegistry::makeMoveOperator(const Goal& goal, const WorldState& world) const {
    if (!world.grid || goal.args.size() < 2) return Operator();
    const std::string& targetSymbol = goal.args[1];
    if (targetSymbol == "Exit" && !(world.robberHasVault || world.vaultStolen)) return Operator();
    Position target = targetPositionForSymbol(targetSymbol, world, *this);
    if (!world.grid->isValid(target) || world.grid->isWall(target)) return Operator();
    Operator op;
    op.name = "MOVE";
    op.effect = Goal("on", {"R", targetSymbol});
    op.preconditions = {Goal("samefloor", {"R", targetSymbol})};
    op.checkPreconditions = [target](const WorldState& w) {
        return w.grid && w.grid->isValid(target) && !w.grid->isWall(target);
    };
    op.applyEffects = [target](WorldState& w) {
        w.robberPos = target;
    };
    return op;
}

Operator OperatorRegistry::makeClimbOperator(const Goal& goal, const WorldState& world) const {
    if (!world.grid || goal.args.size() < 2) return Operator();
    std::string dest = goal.args[1];
    int destFloor = floorOfSymbol(dest, world);
    Operator op;
    op.name = "CLIMB";
    op.effect = Goal("samefloor", {"R", dest});
    op.preconditions = {Goal("on", {"R", "US"})};
    op.checkPreconditions = [destFloor](const WorldState& w) {
        if (!w.grid) return false;
        if (w.robberPos.z >= destFloor) return false;
        return w.grid->getCell(w.robberPos) == CellType::STAIRS;
    };
    op.applyEffects = [destFloor](WorldState& w) {
        w.robberPos.z = destFloor;
    };
    return op;
}

Operator OperatorRegistry::makeDescendOperator(const Goal& goal, const WorldState& world) const {
    if (!world.grid || goal.args.size() < 2) return Operator();
    std::string dest = goal.args[1];
    int destFloor = floorOfSymbol(dest, world);
    Operator op;
    op.name = "DESCEND";
    op.effect = Goal("samefloor", {"R", dest});
    op.preconditions = {Goal("on", {"R", "DS"})};
    op.checkPreconditions = [destFloor](const WorldState& w) {
        if (!w.grid) return false;
        if (w.robberPos.z <= destFloor) return false;
        return w.grid->getCell(w.robberPos) == CellType::ELEVATOR;
    };
    op.applyEffects = [destFloor](WorldState& w) {
        w.robberPos.z = destFloor;
    };
    return op;
}

Operator OperatorRegistry::makeEscapeOperator() const {
    Operator op;
    op.name = "ESCAPE";
    op.effect = Goal("escape", {"R"});
    op.preconditions = {Goal("on", {"R", "Exit"})};
    op.checkPreconditions = [](const WorldState& w) {
        return w.robberPos == w.exitPos;
    };
    op.applyEffects = [](WorldState& w) {
        w.robberWon = true;
    };
    return op;
}

Position OperatorRegistry::findNearestCellOfType(const Grid3D& grid,
                                                 const Position& origin,
                                                 CellType type) const {
    Position best(-1, -1, -1);
    int bestDist = std::numeric_limits<int>::max();
    for (int z = 0; z < grid.getDepth(); ++z) {
        for (int y = 0; y < grid.getHeight(); ++y) {
            for (int x = 0; x < grid.getWidth(); ++x) {
                Position p(x, y, z);
                if (grid.getCell(p) != type) continue;
                int dist = abs(origin.x - x) + abs(origin.y - y) + abs(origin.z - z);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = p;
                }
            }
        }
    }
    return best;
}

Position OperatorRegistry::findNearestAlertZone(const WorldState& world,
                                                const Position& origin) const {
    if (!world.grid) return Position(-1, -1, -1);
    Position best(-1, -1, -1);
    int bestDist = std::numeric_limits<int>::max();
    for (const auto& alertPos : world.grid->getAlertZones()) {
        if (!world.grid->isValid(alertPos)) continue;
        int dist = abs(origin.x - alertPos.x) + abs(origin.y - alertPos.y) + abs(origin.z - alertPos.z);
        if (dist < bestDist) {
            bestDist = dist;
            best = alertPos;
        }
    }
    return best;
}

bool OperatorRegistry::isGoalClear(const Goal& goal, const WorldState& world) const {
    if (goal.predicate != "clear" || goal.args.size() < 1) return false;
    if (!world.grid) return false;
    for (const auto& police : world.policePositions) {
        if (police == world.vaultPos) {
            return false;
        }
    }
    return true;
}

bool OperatorRegistry::isSameFloorTargetAvailable(const Goal& goal, const WorldState& world) const {
    if (!world.grid || goal.args.size() < 2) return false;
    const std::string& target = goal.args[1];
    if (target == "US") {
        Position p = findNearestCellOfType(*world.grid, world.robberPos, CellType::STAIRS);
        return world.grid->isValid(p);
    }
    if (target == "DS") {
        Position p = findNearestCellOfType(*world.grid, world.robberPos, CellType::ELEVATOR);
        return world.grid->isValid(p);
    }
    return true;
}

} // namespace strips
} // namespace ai
