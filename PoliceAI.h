#ifndef POLICEAI_H
#define POLICEAI_H

#include "Agent.h"

using namespace std;

class PoliceAI : public Agent {
public:
  PoliceAI(Position startPos) : Agent(startPos) {}

  void moveTowards(Position target, const Grid &grid, bool vaultCollected,
                   int turn);
};

#endif
