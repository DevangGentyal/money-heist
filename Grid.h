#ifndef GRID_H
#define GRID_H

#include <vector>
#include <string>

using namespace std;

struct Position {
    int x, y, z;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    bool operator<(const Position& other) const {
        if (z != other.z) return z < other.z;
        if (y != other.y) return y < other.y;
        return x < other.x;
    }
};

class Grid {
public:
    vector<vector<vector<char>>> map; // [z][y][x]
    int width, height, depth;
    Position initialRobberPos;
    vector<Position> initialPolicePos;
    vector<Position> initialCCTVPos;
    Position vaultPos;
    Position exitPos;

    Grid();
    void loadLevel(int difficulty);
    bool isWall(int x, int y, int z) const;
    bool isValid(int x, int y, int z) const;
    char getCell(int x, int y, int z) const;
    void setCell(int x, int y, int z, char c);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getDepth() const { return depth; }
    
    Position getInitialRobberPos() const { return initialRobberPos; }
    Position getVaultPos() const { return vaultPos; }
    Position getExitPos() const { return exitPos; }
};

#endif
