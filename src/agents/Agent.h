#ifndef AGENT_H
#define AGENT_H

#include "../grid/Grid3D.h"

using namespace std;

enum class AgentRole {
    ROBBER,
    POLICE
};

class Agent {
protected:
    Position pos;
    Position lastPos;
    AgentRole role;
    
public:
    Agent(const Position& startPos, AgentRole r);
    virtual ~Agent() = default;
    
    Position getPosition() const { return pos; }
    Position getLastPosition() const { return lastPos; }
    void setPosition(const Position& p) {
        lastPos = pos;
        pos = p;
    }
    AgentRole getRole() const { return role; }
    
    virtual bool canMoveTo(const Position& target, const Grid3D& grid) const;
    virtual Position getNextMove(const Grid3D& grid, 
                                const vector<Position>& otherAgents) = 0;
};

#endif
