#include "AStar.h"
#include <algorithm>
#include <iostream>

using namespace std;

int AStar::calculateManhattan(Position p1, Position p2) {
    return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

vector<Position> AStar::findPath(Position start, Position goal, const Grid& grid) {
    priority_queue<Node, vector<Node>, greater<Node>> openList;
    map<pair<int, int>, Position> parentMap;
    map<pair<int, int>, int> gScore;

    openList.push({start, 0, calculateManhattan(start, goal), calculateManhattan(start, goal)});
    gScore[{start.x, start.y}] = 0;

    cout << "\n--- AI Decision Process ---\n";
    cout << "Evaluating available options (successors):\n";

    while (!openList.empty()) {
        Node current = openList.top();
        openList.pop();

        bool isStartNode = (current.pos == start);

        if (current.pos == goal) {
            vector<Position> path;
            Position step = goal;
            while (!(step == start)) {
                path.push_back(step);
                step = parentMap[{step.x, step.y}];
            }
            reverse(path.begin(), path.end());
            return path;
        }

        // Neighbors: up, down, left, right
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};

        bool loggedCurrent = false;

        for (int i = 0; i < 4; ++i) {
            Position neighbor = {current.pos.x + dx[i], current.pos.y + dy[i]};

            if (grid.isValid(neighbor.x, neighbor.y) && !grid.isWall(neighbor.x, neighbor.y)) {
                int tentativeG = current.g + 1;

                if (gScore.find({neighbor.x, neighbor.y}) == gScore.end() || tentativeG < gScore[{neighbor.x, neighbor.y}]) {
                    parentMap[{neighbor.x, neighbor.y}] = current.pos;
                    gScore[{neighbor.x, neighbor.y}] = tentativeG;
                    int h = calculateManhattan(neighbor, goal);
                    int f = tentativeG + h;
                    openList.push({neighbor, tentativeG, h, f});

                    // Debug Logging ONLY for the Police's immediate options
                    if (isStartNode) {
                        cout << "  Option (" << neighbor.x << "," << neighbor.y << ")\n";
                        cout << "  g = " << tentativeG << " | h = " << h << " | f = " << f << "\n";
                        cout << "  Grid Preview:\n";
                        for (int y = 0; y < grid.getHeight(); ++y) {
                            cout << "    ";
                            for (int x = 0; x < grid.getWidth(); ++x) {
                                Position current_pos = {x, y};
                                if (current_pos == goal) {
                                    cout << 'R';
                                } else if (current_pos == neighbor) {
                                    cout << 'P';
                                } else {
                                    cout << grid.getCell(x, y);
                                }
                            }
                            cout << "\n";
                        }
                        cout << "\n";
                    }
                }
            }
        }
    }

    return {}; // No path found
}
