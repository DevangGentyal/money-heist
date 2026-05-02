#include "Planner.h"

Planner::Planner() : planDepth(0) {}

void Planner::recordPlanStep(const string& goal,
                            const string& selectedOp,
                            const vector<string>& preconds,
                            const vector<string>& effs,
                            bool success) {
    PlanStep step;
    step.goalExpression = goal;
    step.selectedOperator = selectedOp;
    step.preconditions = preconds;
    step.effects = effs;
    step.depth = planDepth++;
    step.success = success;
    planLog.push_back(step);
}
