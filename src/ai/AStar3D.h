#ifndef ASTAR3D_H
#define ASTAR3D_H

#include "../grid/Grid3D.h"
#include "../planning/STRIPSOperators.h"
#include "HeuristicEngine.h"
#include <map>
#include <queue>
#include <set>
#include <vector>

using namespace std;

struct Node {
    Position pos;
    int g;
    int h;
    int f;

    bool operator>(const Node& other) const {
        if (f != other.f) return f > other.f;
        return pos < other.pos;
    }
};

struct SearchTrace {
    Position start;
    Position goal;
    vector<Node> immediateSuccessors;
    Position chosenNode;
    bool pathFound;
    bool usedHeuristic;
    bool canTransitionFloors;
    GoalType goalType;

    SearchTrace()
        : chosenNode(), pathFound(false), usedHeuristic(false), canTransitionFloors(true), goalType(GoalType::NONE) {}
};

class AStar3D {
public:
    static vector<Position> findPath(const Position& start,
                                     const Position& goal,
                                     const Grid3D& grid,
                                     GoalType goalType = GoalType::MOVE_FLOOR_GOAL,
                                     bool canTransitionFloors = true,
                                     int initialG = 0
                                    );

    static vector<Position> findPathWithHeuristic(const Position& start,
                                                  const Position& goal,
                                                  const Grid3D& grid,
                                                  HeuristicEngine& heuristic,
                                                  WorldState& world,
                                                  RuleEngine& rules,
                                                  GoalType goalType,
                                                  const vector<Position>& obstacles,
                                                  bool isRobberPerspective,
                                                  bool canTransitionFloors = true,
                                                  int initialG = 0
                                                );

    static int maneuverCost(const Position& from, const Position& to,
                           const Grid3D& grid);

    static const SearchTrace& getLastSearchTrace();
    static vector<Node> getSuccessorLog();
    static void clearLastSearchTrace();
};

#endif
