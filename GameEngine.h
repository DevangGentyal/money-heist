#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "Grid.h"
#include "PoliceAI.h"
#include "Robber.h"

using namespace std;

class GameEngine {
public:
    Grid grid;
    Robber robber;
    PoliceAI police;
    int turnNumber;
    bool gameRunning;

    GameEngine();
    void run();
    void processTurn();
    void checkWinConditions();
};

#endif
