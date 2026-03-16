#ifndef GRID_H
#define GRID_H

#include <vector>
#include <string>

using namespace std;

struct Position {
    int x, y;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

class Grid {
public:
    vector<vector<char>> map;
    int width, height;
    Position initialRobberPos;
    Position initialPolicePos;
    Position vaultPos;
    Position exitPos;

    Grid();
    void loadDefaultMap();
    bool isWall(int x, int y) const;
    bool isValid(int x, int y) const;
    char getCell(int x, int y) const;
    int getWidth() const;
    int getHeight() const;

    Position getInitialRobberPos() const { return initialRobberPos; }
    Position getInitialPolicePos() const { return initialPolicePos; }
    Position getVaultPos() const { return vaultPos; }
    Position getExitPos() const { return exitPos; }
};

#endif
