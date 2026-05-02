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
}

const SearchTrace& AStar3D::getLastSearchTrace() {
    return lastTrace;
}

vector<Node> AStar3D::getSuccessorLog() {
    return lastTrace.immediateSuccessors;
}

void AStar3D::clearLastSearchTrace() {
    lastTrace = SearchTrace();
}

vector<Position> AStar3D::findPath(const Position& start,
                                   const Position& goal,
                                   const Grid3D& grid,
                                   GoalType goalType,
                                   bool canTransitionFloors) {
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
    startNode.g = 0;
    startNode.h = grid.manhattanDistance(start, goal);
    startNode.f = startNode.h;

    for (const auto& neighbor : grid.getNeighbors(start, canTransitionFloors)) {
        int g = maneuverCost(start, neighbor, grid);
        float h = grid.manhattanDistance(neighbor, goal);
        lastTrace.immediateSuccessors.push_back(makeNode(neighbor, g, h));
    }

    openSet.push(startNode);
    gScore[start] = 0;

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

        for (const auto& neighbor : grid.getNeighbors(current.pos, canTransitionFloors)) {
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

    if (!lastTrace.immediateSuccessors.empty()) {
        lastTrace.chosenNode = lastTrace.immediateSuccessors.front().pos;
    }
    return vector<Position>();
}

vector<Position> AStar3D::findPathWithHeuristic(const Position& start,
                                                const Position& goal,
                                                const Grid3D& grid,
                                                HeuristicEngine& heuristic,
                                                WorldState& world,
                                                RuleEngine& rules,
                                                GoalType goalType,
                                                const vector<Position>& obstacles,
                                                bool isRobberPerspective,
                                                bool canTransitionFloors) {
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
    startNode.g = 0;
    startNode.h = heuristic.compute(startNode, goalType, world, rules);
    startNode.f = startNode.h;

    for (const auto& neighbor : grid.getNeighbors(start, canTransitionFloors)) {
        int g = maneuverCost(start, neighbor, grid);
        Node temp;
        temp.pos = neighbor;
        temp.g = g;
        temp.h = heuristic.compute(temp, goalType, world, rules);
        lastTrace.immediateSuccessors.push_back(makeNode(neighbor, g, temp.h));
    }

    openSet.push(startNode);
    gScore[start] = 0;

    int iterations = 0;
    const int MAX_ITERATIONS = 5000;

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

        for (const auto& neighbor : grid.getNeighbors(current.pos, canTransitionFloors)) {
            if (closedSet.count(neighbor)) continue;

            int tentativeG = gScore[current.pos] + maneuverCost(current.pos, neighbor, grid);
            if (!gScore.count(neighbor) || tentativeG < gScore[neighbor]) {
                cameFrom[neighbor] = current.pos;
                gScore[neighbor] = tentativeG;
                Node nextNode;
                nextNode.pos = neighbor;
                nextNode.g = tentativeG;
                nextNode.h = heuristic.compute(nextNode, goalType, world, rules);
                nextNode.f = nextNode.g + nextNode.h;
                openSet.push(nextNode);
            }
        }
    }

    if (!lastTrace.immediateSuccessors.empty()) {
        lastTrace.chosenNode = lastTrace.immediateSuccessors.front().pos;
    }
    return vector<Position>();
}

int AStar3D::maneuverCost(const Position& from, const Position& to,
                         const Grid3D& grid) {
    return grid.getMovementCost(from, to);
}
