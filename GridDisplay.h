#ifndef GRIDDISPLAY_H
#define GRIDDISPLAY_H

#include "GameEngine.h"

class GridDisplay {
public:
    static void clearScreen();
    static void print(const GameEngine& engine);
};

#endif
