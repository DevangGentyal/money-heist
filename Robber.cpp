#include "Robber.h"
#include <iostream>

using namespace std;

bool Robber::move(char input, const Grid& grid) {
    Position nextPos = pos;

    if (input == 'w') nextPos.y--;
    else if (input == 's') nextPos.y++;
    else if (input == 'a') nextPos.x--;
    else if (input == 'd') nextPos.x++;
    else return false;

    if (grid.isValid(nextPos.x, nextPos.y) && !grid.isWall(nextPos.x, nextPos.y)) {
        pos = nextPos;
        return true;
    } else {
        cout << "Invalid move: wall detected\n";
        return false;
    }
}
