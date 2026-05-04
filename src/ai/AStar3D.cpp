#include "AStar3D.h"
#include <algorithm>
#include <cmath>

namespace {
thread_local SearchTrace lastTrace;

Node makeNode(const Position& pos, int g, float h) {
    Node node;
    node.pos = pos;
    node.g = g;
    node.h = h;
    node.f = g + h;
    return node;
}

template <typename HeuristicFn>
void logSuccessors(const Position& current,
                   int currentG,
                   const Grid3D& grid,
                   const vector<Position>& neighbors,
                   bool canTransitionFloors,
                   HeuristicFn&& heuristicFn,
                   bool isStartNode) {
    (void)current;
    (void)canTransitionFloors;
    if (!isStartNode) return;
    lastTrace.immediateSuccessors.clear();
    for (const auto& neighbor : neighbors) {
        int tentativeG = currentG + grid.getMovementCost(current, neighbor);
        float h = heuristicFn(neighbor, tentativeG);
        lastTrace.immediateSuccessors.push_back(makeNode(neighbor, tentativeG, h));
    }
}
} // namespace

const SearchTrace& AStar3D::getLastSearchTrace() { return lastTrace; }
vector<Node> AStar3D::getSuccessorLog() { return lastTrace.immediateSuccessors; }
void AStar3D::clearLastSearchTrace() { lastTrace = SearchTrace(); }

// ─────────────────────────────────────────────────────────────────────────────
//  findPath  — initialG carries the agent's lifetime step count into g(n)
// ─────────────────────────────────────────────────────────────────────────────
vector<Position> AStar3D::findPath(const Position& start,
                                   const Position& goal,
                                   const Grid3D& grid,
                                   GoalType goalType,
                                   bool canTransitionFloors,
                                   int initialG) {          // <-- new param
    clearLastSearchTrace();
    lastTrace.start = start;
    lastTrace.goal = goal;
    lastTrace.usedHeuristic = false;
    lastTrace.canTransitionFloors = canTransitionFloors;
    lastTrace.goalType = goalType;

    priority_queue<Node, vector<Node>, greater<Node>> openSet;
    map<Position, int> gScore;
    map<Position, Position> cameFrom;
    set<Position> closedSet;

    Node startNode;
    startNode.pos = start;
    startNode.g = initialG;                                  // <-- was 0
    startNode.h = grid.manhattanDistance(start, goal);
    startNode.f = startNode.g + startNode.h;

    openSet.push(startNode);
    gScore[start] = initialG;                                // <-- was 0

    bool isFirstExpansion = true;
    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.pos == goal) {
            vector<Position> path;
            Position p = goal;
            while (p != start) {
                path.push_back(p);
                p = cameFrom[p];
            }
            path.push_back(start);
            reverse(path.begin(), path.end());
            lastTrace.pathFound = true;
            lastTrace.chosenNode = path.size() > 1 ? path[1] : start;
            return path;
        }

        if (closedSet.count(current.pos)) continue;
        closedSet.insert(current.pos);

        const vector<Position> neighbors = grid.getNeighbors(current.pos, canTransitionFloors);
        logSuccessors(current.pos, gScore[current.pos], grid, neighbors, canTransitionFloors,
                      [&](const Position& neighbor, int /*tentativeG*/) {
                          return grid.manhattanDistance(neighbor, goal);
                      }, isFirstExpansion);
        isFirstExpansion = false;

        for (const auto& neighbor : neighbors) {
            if (closedSet.count(neighbor)) continue;

            int tentativeG = gScore[current.pos] + maneuverCost(current.pos, neighbor, grid);

            if (!gScore.count(neighbor) || tentativeG < gScore[neighbor]) {
                cameFrom[neighbor] = current.pos;
                gScore[neighbor] = tentativeG;

                Node nextNode;
                nextNode.pos = neighbor;
                nextNode.g = tentativeG;
                nextNode.h = grid.manhattanDistance(neighbor, goal);
                nextNode.f = nextNode.g + nextNode.h;
                openSet.push(nextNode);
            }
        }
    }

    return vector<Position>();
}

// ─────────────────────────────────────────────────────────────────────────────
//  findPathWithHeuristic  — same initialG treatment
// ─────────────────────────────────────────────────────────────────────────────
vector<Position> AStar3D::findPathWithHeuristic(const Position& start,
                                                const Position& goal,
                                                const Grid3D& grid,
                                                HeuristicEngine& heuristic,
                                                WorldState& world,
                                                RuleEngine& rules,
                                                GoalType goalType,
                                                const vector<Position>& obstacles,
                                                bool isRobberPerspective,
                                                bool canTransitionFloors,
                                                int initialG) {          // <-- new param
    clearLastSearchTrace();
    lastTrace.start = start;
    lastTrace.goal = goal;
    lastTrace.usedHeuristic = true;
    lastTrace.canTransitionFloors = canTransitionFloors;
    lastTrace.goalType = goalType;

    priority_queue<Node, vector<Node>, greater<Node>> openSet;
    map<Position, int> gScore;
    map<Position, Position> cameFrom;
    set<Position> closedSet;

    Node startNode;
    startNode.pos = start;
    startNode.g = initialG;                                  // <-- was 0
    startNode.h = grid.manhattanDistance(start, goal);
    startNode.f = startNode.g + startNode.h;

    openSet.push(startNode);
    gScore[start] = initialG;                                // <-- was 0

    int iterations = 0;
    const int MAX_ITERATIONS = 5000;

    bool isFirstExpansion = true;
    while (!openSet.empty() && iterations < MAX_ITERATIONS) {
        iterations++;
        Node current = openSet.top();
        openSet.pop();

        if (current.pos == goal) {
            vector<Position> path;
            Position p = goal;
            while (p != start) {
                path.push_back(p);
                p = cameFrom[p];
            }
            path.push_back(start);
            reverse(path.begin(), path.end());
            lastTrace.pathFound = true;
            lastTrace.chosenNode = path.size() > 1 ? path[1] : start;
            return path;
        }

        if (closedSet.count(current.pos)) continue;
        closedSet.insert(current.pos);

        const vector<Position> neighbors = grid.getNeighbors(current.pos, canTransitionFloors);
        logSuccessors(current.pos, gScore[current.pos], grid, neighbors, canTransitionFloors,
                      [&](const Position& neighbor, int /*tentativeG*/) {
                          return grid.manhattanDistance(neighbor, goal);
                      }, isFirstExpansion);
        isFirstExpansion = false;

        for (const auto& neighbor : neighbors) {
            if (closedSet.count(neighbor)) continue;

            int tentativeG = gScore[current.pos] + maneuverCost(current.pos, neighbor, grid);
            if (!gScore.count(neighbor) || tentativeG < gScore[neighbor]) {
                cameFrom[neighbor] = current.pos;
                gScore[neighbor] = tentativeG;
                Node nextNode;
                nextNode.pos = neighbor;
                nextNode.g = tentativeG;
                nextNode.h = grid.manhattanDistance(neighbor, goal);
                nextNode.f = nextNode.g + nextNode.h;
                openSet.push(nextNode);
            }
        }
    }

    return vector<Position>();
}

int AStar3D::maneuverCost(const Position& from, const Position& to, const Grid3D& grid) {
    return grid.getMovementCost(from, to);
}