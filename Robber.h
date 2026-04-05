#ifndef ROBBER_H
#define ROBBER_H

#include "Agent.h"

class Robber : public Agent {
public:
    bool hasVault;

    Robber();
    Robber(Position startPos);
    
    bool move(char direction, const Grid& grid);
};

#endif
