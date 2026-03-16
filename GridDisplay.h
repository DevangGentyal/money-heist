#ifndef GRIDDISPLAY_H
#define GRIDDISPLAY_H

#include "Grid.h"
#include <iostream>

using namespace std;

class GridDisplay {
public:
    static void print(const Grid& grid, Position robberPos, Position policePos, bool vaultCollected, int turn);
};

#endif
