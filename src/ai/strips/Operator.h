#pragma once

#include <functional>
#include <string>
#include <vector>
#include "../../planning/STRIPSOperators.h"

namespace ai {
namespace strips {

struct Goal {
    std::string predicate;
    std::vector<std::string> args;

    Goal() = default;
    Goal(std::string pred, std::vector<std::string> argList)
        : predicate(std::move(pred)), args(std::move(argList)) {}

    std::string toString() const;
    static Goal parse(const std::string& expression);
    bool operator==(const Goal& other) const {
        return predicate == other.predicate && args == other.args;
    }
};

struct Operator {
    std::string name;
    Goal effect;
    std::vector<Goal> preconditions;
    std::function<bool(const WorldState&)> checkPreconditions;
    std::function<void(WorldState&)> applyEffects;
};

} // namespace strips
} // namespace ai
