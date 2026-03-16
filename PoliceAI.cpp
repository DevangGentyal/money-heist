#include "PoliceAI.h"
#include "AStar.h"
#include <iostream>

using namespace std;

void PoliceAI::moveTowards(Position target, const Grid& grid) {
    vector<Position> path = AStar::findPath(pos, target, grid);

    if (!path.empty()) {
        pos = path[0]; // Move one step along the path
        cout << "\n  [✓] Best node selected -> (" << pos.x << "," << pos.y << ")\n";
        cout << "  [➡️] Police actually moves to (" << pos.x << "," << pos.y << ")\n";
        cout << "-----------------------------------\n";
    } else {
        cout << "\n  [!] Police is stuck! No path found to (" << target.x << "," << target.y << ")\n";
        cout << "-----------------------------------\n";
    }
}
