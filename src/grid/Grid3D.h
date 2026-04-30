#ifndef GRID3D_H
#define GRID3D_H

#include <vector>
#include <string>
#include <set>

using namespace std;

struct Position {
    int x, y, z;
    
    Position() : x(0), y(0), z(0) {}
    Position(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    
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

enum class CellType {
    EMPTY = 0,
    WALL = 1,
    VAULT = 2,
    EXIT = 3,
    STAIRS = 4,
    ELEVATOR = 5,
    CCTV_ZONE = 6,
    ALERT_ZONE = 7
};

class Grid3D {
private:
    vector<vector<vector<CellType>>> map; // [z][y][x]
    int width, height, depth;
    
    // Key positions
    Position initialRobberPos;
    vector<Position> initialPolicePos;
    Position vaultPos;
    Position exitPos;
    vector<Position> cctvPositions;
    vector<Position> alertZones;

public:
    Grid3D();
    void loadLevel(int difficulty);
    
    // Validation
    bool isWall(const Position& p) const;
    bool isValid(const Position& p) const;
    bool isCCTVZone(const Position& p) const;
    bool isAlertZone(const Position& p) const;
    
    // Cell access
    CellType getCell(const Position& p) const;
    void setCell(const Position& p, CellType type);
    
    // Getters
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getDepth() const { return depth; }
    Position getInitialRobberPos() const { return initialRobberPos; }
    vector<Position> getInitialPolicePos() const { return initialPolicePos; }
    Position getVaultPos() const { return vaultPos; }
    Position getExitPos() const { return exitPos; }
    vector<Position> getCCTVPositions() const { return cctvPositions; }
    vector<Position> getAlertZones() const { return alertZones; }
    
    // Distance calculations
    int manhattanDistance(const Position& p1, const Position& p2) const;
    int euclideanDistance(const Position& p1, const Position& p2) const;
    int verticalCost(const Position& p1, const Position& p2) const;
    
    // Get neighbors for pathfinding
    vector<Position> getNeighbors(const Position& p, bool allowFloorTransitions = true) const;
    int getMovementCost(const Position& from, const Position& to) const;
};

#endif
