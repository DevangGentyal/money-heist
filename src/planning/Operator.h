#ifndef OPERATOR_H
#define OPERATOR_H

#include <string>
#include <vector>
#include "Goal.h"
#include "Precondition.h"
#include "../grid/Grid3D.h"

using namespace std;

/**
 * Operator represents an action in STRIPS-style planning.
 * 
 * Structure:
 * - Name: what the operator does
 * - Preconditions: what must be true before execution
 * - Effects: what happens after execution
 * - Subgoals: intermediate goals needed to enable this operator
 * 
 * Example:
 * Operator: "move(police, target)"
 * Preconditions: [path exists, not wall]
 * Effects: [position updated, moved closer to goal]
 * Subgoals: []
 */
class Operator {
public:
    struct Effect {
        string description;
        bool positive;  // true = add, false = delete
        
        Effect(const string& desc, bool pos = true)
            : description(desc), positive(pos) {}
    };

    string name;                           // Operator name (e.g., "move", "goToFloor")
    string description;                    // Human-readable description
    vector<Precondition> preconditions;   // Must all be satisfied
    vector<Effect> effects;               // Results of applying operator
    vector<Goal> subgoals;                // Goals to achieve first
    int cost;                             // Cost of this operator

    Operator() : cost(1) {}
    
    Operator(const string& n, const string& desc)
        : name(n), description(desc), cost(1) {}

    /**
     * Check if all preconditions are satisfied
     */
    bool canExecute(const Grid3D& grid,
                   const Position& agentPos,
                   const Position& otherAgentPos = Position(),
                   bool robberHasVault = false) const;

    /**
     * Add a precondition to this operator
     */
    void addPrecondition(const Precondition& pre);

    /**
     * Add an effect to this operator
     */
    void addEffect(const Effect& eff);

    /**
     * Add a subgoal (what to achieve before this operator)
     */
    void addSubgoal(const Goal& goal);

    /**
     * Get all preconditions as readable strings
     */
    vector<string> getPreconditionDescriptions() const;

    /**
     * Get all effects as readable strings
     */
    vector<string> getEffectDescriptions() const;

    /**
     * Get all subgoals as readable strings
     */
    vector<string> getSubgoalExpressions() const;
};

#endif
