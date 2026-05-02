#include "Operator.h"

bool Operator::canExecute(const Grid3D& grid,
                         const Position& agentPos,
                         const Position& otherAgentPos,
                         bool robberHasVault) const {
    // All preconditions must be satisfied
    for (const auto& pre : preconditions) {
        if (!pre.isSatisfied(grid, agentPos, otherAgentPos, robberHasVault)) {
            return false;
        }
    }
    return true;
}

void Operator::addPrecondition(const Precondition& pre) {
    preconditions.push_back(pre);
}

void Operator::addEffect(const Effect& eff) {
    effects.push_back(eff);
}

void Operator::addSubgoal(const Goal& goal) {
    subgoals.push_back(goal);
}

vector<string> Operator::getPreconditionDescriptions() const {
    vector<string> descs;
    for (const auto& pre : preconditions) {
        descs.push_back(pre.toString());
    }
    return descs;
}

vector<string> Operator::getEffectDescriptions() const {
    vector<string> descs;
    for (const auto& eff : effects) {
        descs.push_back((eff.positive ? "+ " : "- ") + eff.description);
    }
    return descs;
}

vector<string> Operator::getSubgoalExpressions() const {
    vector<string> exprs;
    for (const auto& goal : subgoals) {
        exprs.push_back(goal.toString());
    }
    return exprs;
}
