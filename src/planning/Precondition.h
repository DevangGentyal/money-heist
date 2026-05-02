#ifndef PRECONDITION_H
#define PRECONDITION_H

#include <string>
#include <vector>
#include "../grid/Grid3D.h"

using namespace std;

/**
 * Precondition represents a requirement that must be satisfied
 * before an operator can be executed.
 * 
 * Examples:
 * - sameFloor(police, robber)
 * - pathExists(from, to)
 * - at(stairs)
 * - not(in_alert_zone)
 */
class Precondition {
public:
    enum class Type {
        SAME_FLOOR,           // Two agents on same floor
        SAME_CELL,            // Two agents in same cell
        PATH_EXISTS,          // Path exists between two positions
        AT_POSITION,          // Agent at specific position
        AT_STAIRS,            // Agent at stairs
        AT_ELEVATOR,          // Agent at elevator
        HAS_VAULT,            // Robber has vault
        NOT_IN_ALERT_ZONE,    // Not detected by alert zone
        GOAL_SATISFIED        // A goal is already satisfied
    };

    Type type;
    string description;
    Position pos1, pos2;      // For position-based preconditions
    bool hasVault;            // For vault checks

    Precondition() : type(Type::SAME_FLOOR), hasVault(false) {}
    
    Precondition(Type t, const string& desc)
        : type(t), description(desc), hasVault(false) {}

    /**
     * Check if this precondition is satisfied
     */
    bool isSatisfied(const Grid3D& grid,
                    const Position& agentPos,
                    const Position& otherAgentPos = Position(),
                    bool robberHasVault = false) const;
    
    string toString() const { return description; }

private:
    bool checkSameFloor(const Position& p1, const Position& p2) const;
    bool checkSameCell(const Position& p1, const Position& p2) const;
    bool checkPathExists(const Grid3D& grid, 
                        const Position& p1, 
                        const Position& p2) const;
    bool checkAtPosition(const Position& p1, const Position& p2) const;
    bool checkAtStairs(const Grid3D& grid, const Position& pos) const;
    bool checkAtElevator(const Grid3D& grid, const Position& pos) const;
};

#endif
