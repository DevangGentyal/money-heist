#include "CCTV.h"
#include <cmath>

CCTV::CCTV() : Agent({0, 0, 0}), turnCounter(0), currentDir(ViewDirection::NORTH), range(4) {}

CCTV::CCTV(Position startPos) : Agent(startPos), turnCounter(0), currentDir(ViewDirection::NORTH), range(4) {}

void CCTV::updateTurn() {
    turnCounter++;
    if (turnCounter % 3 == 0) {
        // Rotate 90 degrees every 3 turns
        if (currentDir == ViewDirection::NORTH) currentDir = ViewDirection::EAST;
        else if (currentDir == ViewDirection::EAST) currentDir = ViewDirection::SOUTH;
        else if (currentDir == ViewDirection::SOUTH) currentDir = ViewDirection::WEST;
        else if (currentDir == ViewDirection::WEST) currentDir = ViewDirection::NORTH;
    }
}

bool CCTV::isSpotted(Position target, const Grid& grid) {
    if (target.z != pos.z) return false; // Can only see same floor
    
    int dx = target.x - pos.x;
    int dy = target.y - pos.y;
    
    // Check range
    if (abs(dx) > range || abs(dy) > range) return false;
    
    // Check direction cone (FOV)
    bool inFOV = false;
    if (currentDir == ViewDirection::NORTH && dy < 0 && abs(dx) <= abs(dy)) inFOV = true;
    if (currentDir == ViewDirection::SOUTH && dy > 0 && abs(dx) <= abs(dy)) inFOV = true;
    if (currentDir == ViewDirection::EAST && dx > 0 && abs(dy) <= abs(dx)) inFOV = true;
    if (currentDir == ViewDirection::WEST && dx < 0 && abs(dy) <= abs(dx)) inFOV = true;
    
    if (!inFOV) return false;
    
    // Bresenham's Line Algorithm for Line-Of-Sight
    int x0 = pos.x, y0 = pos.y;
    int x1 = target.x, y1 = target.y;
    
    int diff_x = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int diff_y = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
    int err = diff_x + diff_y, e2; /* error value e_xy */

    for (;;) {
        if (x0 == x1 && y0 == y1) break;
        if (grid.isWall(x0, y0, pos.z) && !(x0 == pos.x && y0 == pos.y)) return false; // Wall blocks view
        e2 = 2 * err;
        if (e2 >= diff_y) { err += diff_y; x0 += sx; }
        if (e2 <= diff_x) { err += diff_x; y0 += sy; }
    }
    
    return true; // Line of sight is clear
}
