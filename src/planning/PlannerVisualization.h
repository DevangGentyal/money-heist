#ifndef PLANNER_VISUALIZATION_H
#define PLANNER_VISUALIZATION_H

#include <string>
#include <vector>
#include "Goal.h"
#include "Operator.h"
#include "PlannerLog.h"
#include "../ai/AStar3D.h"

using namespace std;

/**
 * PlannerVisualization generates formatted strings for displaying planning information
 * in the right panel of the UI.
 * 
 * Displays:
 * - Goal Stack (top 5 goals)
 * - Current Goal Details
 * - Selected Operator
 * - Preconditions + Status
 * - Effects
 * - A* Node Evaluations (if available)
 * - Final Decision Reasoning
 */
class PlannerVisualization {
public:
    /**
     * Format goal stack for display
     */
    static string formatGoalStack(const vector<Goal>& goals, int maxDisplay = 5);
    
    /**
     * Format current goal with details
     */
    static string formatCurrentGoal(const Goal& goal);
    
    /**
     * Format operator selection with preconditions
     */
    static string formatOperatorSelection(const Operator& op,
                                         const vector<bool>& preconditionStatus);
    
    /**
     * Format A* node evaluations
     */
    static string formatAStarEvaluations(const vector<string>& nodeEvals);
    static string formatAStarTrace(const SearchTrace& trace);
    
    /**
     * Format decision log entry for display
     */
    static string formatDecisionLog(const PlannerLog::DecisionRecord& record);
    
    /**
     * Format a simple one-liner status message
     */
    static string formatStatusMessage(const Goal& currentGoal,
                                     const string& operator_name,
                                     int stackDepth);
    
    /**
     * Create full right-panel display content
     */
    static string createFullPanelContent(const vector<Goal>& goalStack,
                                        const Goal& currentGoal,
                                        const string& selectedOperator,
                                        const vector<string>& preconditions,
                                        const vector<string>& effects,
                                        const SearchTrace& trace,
                                        const string& reasoning);

private:
    static string indent(const string& str, int spaces);
    static string getGoalTypeSymbol(Goal::Type type);
    static string formatBool(bool value);
};

#endif
