#ifndef ROBBER_H
#define ROBBER_H

#include "Agent.h"

using namespace std;

class Robber : public Agent {
public:
    bool vaultCollected;

    Robber(Position startPos) : Agent(startPos), vaultCollected(false) {}

    bool move(char input, const Grid& grid);
    bool hasVault() const { return vaultCollected; }
    void collectVault() { vaultCollected = true; }
};

#endif
