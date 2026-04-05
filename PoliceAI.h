#ifndef POLICEAI_H
#define POLICEAI_H

#include "Agent.h"
#include "AStar.h"

enum class AIState { PATROL, ALERT, CHASE };

class PoliceAI : public Agent {
public:
    AIState state;
    Position lastKnownTarget;

    PoliceAI();
    PoliceAI(Position startPos);
    
    bool takeTurn(Position targetPos, bool isAlerted, Position alertPos, const Grid& grid);
};

#endif
