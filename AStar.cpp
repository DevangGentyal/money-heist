#include "AStar.h"
#include <cmath>
#include <algorithm>

using namespace std;

int AStar::calculateManhattan(Position p1, Position p2) {
    // 3D Manhattan: distance on floor + cost to switch floors (z diff). Floor switch is usually longer.
    return abs(p1.x - p2.x) + abs(p1.y - p2.y) + abs(p1.z - p2.z) * 5;
}

vector<Position> AStar::findPath(Position start, Position goal, const Grid& grid) {
    priority_queue<Node, vector<Node>, greater<Node>> openSet;
    map<Position, Position> cameFrom;
    map<Position, int> gScore;

    openSet.push({start, 0, calculateManhattan(start, goal), calculateManhattan(start, goal)});
    gScore[start] = 0;

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.pos == goal) {
            vector<Position> path;
            Position curr = goal;
            while (curr != start) {
                path.push_back(curr);
                if (cameFrom.find(curr) == cameFrom.end()) break;
                curr = cameFrom[curr];
            }
            reverse(path.begin(), path.end());
            return path;
        }

        vector<Position> neighbors;
        
        // Horizontal neighbors
        for (int i = 0; i < 4; ++i) {
            Position neighborPos = {current.pos.x + dx[i], current.pos.y + dy[i], current.pos.z};
            if (!grid.isWall(neighborPos.x, neighborPos.y, neighborPos.z)) {
                neighbors.push_back(neighborPos);
            }
        }
        
        // Vertical neighbors (Stairs)
        char currentCell = grid.getCell(current.pos.x, current.pos.y, current.pos.z);
        if (currentCell == 'U') {
            if (grid.isValid(current.pos.x, current.pos.y, current.pos.z + 1) && !grid.isWall(current.pos.x, current.pos.y, current.pos.z + 1)) {
                neighbors.push_back({current.pos.x, current.pos.y, current.pos.z + 1});
            }
        } else if (currentCell == 'D') {
            if (grid.isValid(current.pos.x, current.pos.y, current.pos.z - 1) && !grid.isWall(current.pos.x, current.pos.y, current.pos.z - 1)) {
                neighbors.push_back({current.pos.x, current.pos.y, current.pos.z - 1});
            }
        }

        for (Position& neighbor : neighbors) {
            int tentative_gScore = gScore[current.pos] + 1;
            
            if (gScore.find(neighbor) == gScore.end() || tentative_gScore < gScore[neighbor]) {
                gScore[neighbor] = tentative_gScore;
                int fScore = tentative_gScore + calculateManhattan(neighbor, goal);
                openSet.push({neighbor, tentative_gScore, calculateManhattan(neighbor, goal), fScore});
                cameFrom[neighbor] = current.pos;
            }
        }
    }

    return {}; // Empty path if not found
}
