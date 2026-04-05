#ifndef CCTV_H
#define CCTV_H

#include "Agent.h"

enum class ViewDirection { NORTH, EAST, SOUTH, WEST };

class CCTV : public Agent {
private:
    int turnCounter;
public:
    ViewDirection currentDir;
    int range;

    CCTV();
    CCTV(Position startPos);
    
    void updateTurn();
    bool isSpotted(Position target, const Grid& grid);
};

#endif
