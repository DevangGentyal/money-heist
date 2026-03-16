#ifndef AGENT_H
#define AGENT_H

#include "Grid.h"

using namespace std;

class Agent {
public:
    Position pos;

    Agent(Position startPos) : pos(startPos) {}

    Position getPosition() const { return pos; }
    void setPosition(Position p) { pos = p; }
};

#endif
