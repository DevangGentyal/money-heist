#ifndef GOAL_H
#define GOAL_H

#include <string>
#include <vector>
#include "../grid/Grid3D.h"

using namespace std;

/**
 * Goal represents a single objective in the goal stack planning system.
 * Goals can be:
 * - Primitive (moveTo, catch, escape)
 * - Compound (reachable, sameFloor)
 * 
 * Each goal has:
 * - A goal expression (e.g., "catch(police, robber)")
 * - Satisfaction criteria
 * - Associated data (positions, targets, etc.)
 */
class Goal {
public:
    enum class Type {
        CATCH,              // Police catches robber
        SAME_CELL,          // Police and robber in same cell
        SAME_FLOOR,         // Police and robber on same floor
        REACH_VAULT,        // Robber reaches vault
        COLLECT_VAULT,      // Robber collects vault
        REACH_EXIT,         // Robber reaches exit
        ESCAPE,             // Robber escapes with vault
        MOVE_TO,            // Move to target position
        GO_TO_FLOOR,        // Move to target floor
        AVOID_POLICE,       // Robber avoids police
        PATROL              // Police patrols area
    };

    Type type;
    string expression;          // Human-readable goal expression
    Position targetPos;         // For position-based goals
    int targetFloor;           // For floor-based goals
    Position agentPos;         // Current position of planning agent
    Position otherAgentPos;    // Position of other agent (for relative goals)

    Goal() : type(Type::MOVE_TO), expression(""), targetFloor(-1) {}
    
    Goal(Type t, const string& expr) 
        : type(t), expression(expr), targetFloor(-1) {}
    
    Goal(Type t, const string& expr, const Position& target)
        : type(t), expression(expr), targetPos(target), targetFloor(-1) {}
    
    Goal(Type t, const string& expr, int floor)
        : type(t), expression(expr), targetFloor(floor) {}

    /**
     * Check if this goal is satisfied in current grid state
     */
    bool isSatisfied(const Grid3D& grid, 
                    const Position& agentPos,
                    const Position& otherAgentPos = Position()) const;
    
    /**
     * Get string representation for logging
     */
    string toString() const { return expression; }
    
    bool operator==(const Goal& other) const {
        return type == other.type && expression == other.expression;
    }

private:
    bool checkCatch(const Grid3D& grid, 
                   const Position& police, 
                   const Position& robber) const;
    bool checkSameCell(const Position& p1, const Position& p2) const;
    bool checkSameFloor(const Position& p1, const Position& p2) const;
    bool checkReachVault(const Grid3D& grid, const Position& pos) const;
    bool checkReachExit(const Grid3D& grid, const Position& pos) const;
    bool checkAtTarget(const Position& pos) const;
};

#endif
