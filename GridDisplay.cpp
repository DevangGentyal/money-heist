#include "GridDisplay.h"

using namespace std;

void GridDisplay::print(const Grid& grid, Position robberPos, Position policePos, bool vaultCollected, int turn) {
    cout << "\nTurn: " << turn << "\n\n";

    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            Position current = {x, y};
            if (current == robberPos) {
                cout << 'R';
            } else if (current == policePos) {
                cout << 'P';
            } else if (vaultCollected && current == grid.getVaultPos()) {
                cout << '.';
            } else {
                cout << grid.getCell(x, y);
            }
        }
        cout << "\n";
    }

    cout << "\nRobber Position: (" << robberPos.x << "," << robberPos.y << ")\n";
    cout << "Police Position: (" << policePos.x << "," << policePos.y << ")\n";
    cout << "Vault Collected: " << (vaultCollected ? "YES" : "NO") << "\n";
    cout << "---------------------------\n";
}
