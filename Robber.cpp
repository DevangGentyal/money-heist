#include "Robber.h"

Robber::Robber() : Agent({0, 0, 0}), hasVault(false) {}

Robber::Robber(Position startPos) : Agent(startPos), hasVault(false) {}

bool Robber::move(char direction, const Grid& grid) {
    Position nextPos = pos;

    if (direction == 'w') nextPos.y--;
    else if (direction == 's') nextPos.y++;
    else if (direction == 'a') nextPos.x--;
    else if (direction == 'd') nextPos.x++;
    else if (direction == 'u') { // go UP stairs
        if (grid.getCell(pos.x, pos.y, pos.z) == 'U' && grid.isValid(pos.x, pos.y, pos.z + 1) && !grid.isWall(pos.x, pos.y, pos.z + 1)) {
            nextPos.z++;
        }
    }
    else if (direction == 'j') { // go DOWN stairs
        if (grid.getCell(pos.x, pos.y, pos.z) == 'D' && grid.isValid(pos.x, pos.y, pos.z - 1) && !grid.isWall(pos.x, pos.y, pos.z - 1)) {
            nextPos.z--;
        }
    }

    if (!grid.isWall(nextPos.x, nextPos.y, nextPos.z)) {
        pos = nextPos;
        return true;
    }
    return false;
}
