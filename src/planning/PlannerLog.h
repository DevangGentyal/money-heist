#ifndef PLANNER_LOG_H
#define PLANNER_LOG_H

#include <string>
#include <vector>
#include <sstream>
#include "../grid/Grid3D.h"
#include "Goal.h"
#include "Operator.h"

using namespace std;

/**
 * PlannerLog captures detailed information about planning decisions
 * for visualization and debugging.
 * 
 * Logs each decision with:
 * - Current goal
 * - Available operators
 * - Selected operator
 * - Precondition evaluations
 * - Effects
 * - A* node evaluations (if movement)
 * - Final decision reasoning
 */
class PlannerLog {
public:
    struct DecisionRecord {
        int turnNumber;
        string agentType;                // "Police" or "Robber"
        Position agentPos;
        Position targetPos;
        
        // Goal Stack state
        string currentGoal;
        vector<string> goalStackTop3;   // Top 3 goals in stack
        
        // Operator selection
        string selectedOperator;
        vector<string> preconditions;
        vector<bool> preconditionStatus;  // Which preconditions passed
        vector<string> effects;
        
        // Movement planning
        vector<string> astarNodeEvaluations;  // "Node (x,y,z): g=10 h=15 f=25"
        string finalDecision;
        string reasoning;
        
        int frameCount;  // For animation timing
    };

    // Record a planning decision
    void recordDecision(const DecisionRecord& record);
    
    // Get all records
    const vector<DecisionRecord>& getRecords() const { return records; }
    
    // Get latest record
    const DecisionRecord* getLatestRecord() const;
    
    // Get records for a specific turn
    vector<DecisionRecord> getRecordsForTurn(int turn) const;
    
    // Clear log
    void clear() { records.clear(); }
    
    // Export to string for debugging
    string exportToString() const;
    
    // Export as CSV for analysis
    string exportAsCSV() const;
    
    // Get formatted log entry for display
    string formatDecisionForDisplay(const DecisionRecord& rec) const;
    
    // Get size
    int size() const { return (int)records.size(); }

private:
    vector<DecisionRecord> records;
    
    string formatPosition(const Position& p) const;
    string formatPreconditionStatus(const vector<string>& preconds,
                                   const vector<bool>& status) const;
};

#endif
