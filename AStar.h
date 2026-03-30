#ifndef ASTAR_H
#define ASTAR_H

#include "Grid.h"
#include <map>
#include <queue>
#include <vector>

using namespace std;

struct Node {
  Position pos;
  int g; // Cost from start
  int h; // Heuristic (Manhattan distance)
  int f; // g + h

  bool operator>(const Node &other) const { return f > other.f; }
};

class AStar {
public:
  static vector<Position> findPath(Position start, Position goal,
                                   const Grid &grid, bool vaultCollected,
                                   int turn);
  static int calculateManhattan(Position p1, Position p2);
};

#endif
