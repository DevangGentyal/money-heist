#ifndef PLANNER_H
#define PLANNER_H

#include <string>
#include <vector>

#include "GoalStack.h"
#include "../grid/Grid3D.h"

using namespace std;

class Planner {
public:
    struct PlanStep {
        string goalExpression;
        string selectedOperator;
        vector<string> preconditions;
        vector<string> effects;
        int depth;
        bool success;

        PlanStep() : depth(0), success(false) {}
    };

protected:
    GoalStack goalStack;
    vector<PlanStep> planLog;
    int planDepth;

public:
    Planner();
    virtual ~Planner() {}

    GoalStack& getGoalStack() { return goalStack; }
    const GoalStack& getGoalStack() const { return goalStack; }
    const vector<PlanStep>& getPlanLog() const { return planLog; }
    void clearPlanLog() { planLog.clear(); }

protected:
    void recordPlanStep(const string& goal,
                       const string& selectedOp,
                       const vector<string>& preconds,
                       const vector<string>& effs,
                       bool success);
};

#endif
