#ifndef ASTAR3D_H
#define ASTAR3D_H

#include "../grid/Grid3D.h"
#include "HeuristicEngine.h"
#include <vector>
#include <queue>
#include <map>
#include <set>

using namespace std;

struct Node {
    Position pos;
    int g; // Cost from start
    float h; // Heuristic
    float f; // g + h

    bool operator>(const Node& other) const {
        if (f != other.f) return f > other.f;
        return pos < other.pos; // Tiebreaker
    }
};

class AStar3D {
public:
    static vector<Position> findPath(const Position& start, 
                                     const Position& goal, 
                                     const Grid3D& grid,
                                     bool canTransitionFloors = true);
    
    static vector<Position> findPathWithHeuristic(const Position& start,
                                                  const Position& goal,
                                                  const Grid3D& grid,
                                                  HeuristicEngine& heuristic,
                                                  const vector<Position>& obstacles,
                                                  bool isRobberPerspective,
                                                  bool canTransitionFloors = true);
    
    static int maneuverCost(const Position& from, const Position& to,
                           const Grid3D& grid);
};

#endif
