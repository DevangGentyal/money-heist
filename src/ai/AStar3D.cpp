#include "AStar3D.h"
#include <cmath>
#include <algorithm>

vector<Position> AStar3D::findPath(const Position& start, 
                                   const Position& goal, 
                                   const Grid3D& grid,
                                   bool canTransitionFloors) {
    // Basic A* without custom heuristic
    priority_queue<Node, vector<Node>, greater<Node>> openSet;
    map<Position, int> gScore;
    map<Position, Position> cameFrom;
    set<Position> closedSet;
    
    Node startNode;
    startNode.pos = start;
    startNode.g = 0;
    startNode.h = grid.manhattanDistance(start, goal);
    startNode.f = startNode.h;
    
    openSet.push(startNode);
    gScore[start] = 0;
    
    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();
        
        if (current.pos == goal) {
            // Reconstruct path
            vector<Position> path;
            Position p = goal;
            while (p != start) {
                path.push_back(p);
                p = cameFrom[p];
            }
            path.push_back(start);
            reverse(path.begin(), path.end());
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
    
    return vector<Position>(); // No path found
}

vector<Position> AStar3D::findPathWithHeuristic(const Position& start,
                                                const Position& goal,
                                                const Grid3D& grid,
                                                HeuristicEngine& heuristic,
                                                const vector<Position>& obstacles,
                                                bool isRobberPerspective,
                                                bool canTransitionFloors) {
    priority_queue<Node, vector<Node>, greater<Node>> openSet;
    map<Position, int> gScore;
    map<Position, Position> cameFrom;
    set<Position> closedSet;
    
    Node startNode;
    startNode.pos = start;
    startNode.g = 0;
    startNode.h = heuristic.calculateHeuristic(start, goal, obstacles, isRobberPerspective, canTransitionFloors);
    startNode.f = startNode.h;
    
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
                
                float h = heuristic.calculateHeuristic(neighbor, goal, obstacles, isRobberPerspective, canTransitionFloors);
                
                Node nextNode;
                nextNode.pos = neighbor;
                nextNode.g = tentativeG;
                nextNode.h = h;
                nextNode.f = nextNode.g + nextNode.h;
                
                openSet.push(nextNode);
            }
        }
    }
    
    return vector<Position>();
}

int AStar3D::maneuverCost(const Position& from, const Position& to,
                         const Grid3D& grid) {
    return grid.getMovementCost(from, to);
}
