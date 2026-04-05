#ifndef ASTAR_H
#define ASTAR_H

#include "Grid.h"
#include <vector>
#include <queue>
#include <map>

using namespace std;

struct Node {
    Position pos;
    int g; // Cost from start
    int h; // Heuristic
    int f; // g + h

    bool operator>(const Node& other) const {
        return f > other.f;
    }
};

class AStar {
public:
    static vector<Position> findPath(Position start, Position goal, const Grid& grid);
    static int calculateManhattan(Position p1, Position p2);
};

#endif
