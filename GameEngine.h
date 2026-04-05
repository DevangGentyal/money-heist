#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "Grid.h"
#include "PoliceAI.h"
#include "Robber.h"
#include "CCTV.h"
#include <vector>

using namespace std;

class GameEngine {
public:
    Grid grid;
    Robber robber;
    vector<PoliceAI> policeList;
    vector<CCTV> cctvList;
    int turnNumber;
    bool gameRunning;
    int difficulty;
    bool systemAlerted;
    Position alertPos;
    string gameStatusMessage;

    GameEngine();
    void setDifficulty(int diff);
    void run();
    void processTurn(char input);
    void checkWinConditions();
};

#endif
