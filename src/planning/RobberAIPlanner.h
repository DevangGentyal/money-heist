#ifndef ROBBER_AI_PLANNER_H
#define ROBBER_AI_PLANNER_H

#include "../ai/planner/GoalStackPlanner.h"

class RobberAIPlanner : public ai::planner::GoalStackPlanner {
public:
    explicit RobberAIPlanner(int robberId = 0);
    virtual ~RobberAIPlanner() = default;
};

#endif
