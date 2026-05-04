#ifndef AGENT_H
#define AGENT_H

#include "../grid/Grid3D.h"

using namespace std;

enum class AgentRole {
    ROBBER,
    POLICE
};

class Agent {
    
public:
    Agent(const Position& startPos, AgentRole r);
    virtual ~Agent() = default;
    
    Position getPosition() const { return pos; }
    Position getLastPosition() const { return lastPos; }
    int getStepsTaken() const { return stepsTaken; }
    void setPosition(const Position& p) {
        if (!(pos == p)) {
            lastPos = pos;
            ++stepsTaken;
            pos = p;
        } else {
            lastPos = pos;
            pos = p;
        }
    }
    AgentRole getRole() const { return role; }
    
    virtual bool canMoveTo(const Position& target, const Grid3D& grid) const;
    virtual Position getNextMove(const Grid3D& grid, 
                                const vector<Position>& otherAgents) = 0;

    // Call this after every confirmed move to advance lifetime step count
    void commitMove(const Position& newPos) {
        lastPos = pos;
        pos = newPos;
        stepsTaken++;
    }

    Position pos;
    Position lastPos;
    AgentRole role;
    int stepsTaken;
};

#endif
