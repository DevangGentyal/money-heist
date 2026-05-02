#include "PlannerVisualization.h"
#include <sstream>
#include <iomanip>

string PlannerVisualization::formatGoalStack(const vector<Goal>& goals, int maxDisplay) {
    stringstream ss;
    ss << "GOAL STACK (Top " << min((int)goals.size(), maxDisplay) << ")\n";
    ss << "─────────────────────────────\n";
    
    int displayed = 0;
    for (const auto& goal : goals) {
        if (displayed >= maxDisplay) break;
        ss << "  " << getGoalTypeSymbol(goal.type) << " " 
           << goal.toString() << "\n";
        displayed++;
    }
    
    if (goals.size() > maxDisplay) {
        ss << "  ... (" << (goals.size() - maxDisplay) << " more)\n";
    }
    
    ss << "\n";
    return ss.str();
}

string PlannerVisualization::formatCurrentGoal(const Goal& goal) {
    stringstream ss;
    ss << "CURRENT GOAL\n";
    ss << "─────────────────────────────\n";
    ss << "  Expression: " << goal.toString() << "\n";
    ss << "  Type: " << (int)goal.type << "\n";
    ss << "\n";
    return ss.str();
}

string PlannerVisualization::formatOperatorSelection(const Operator& op,
                                                    const vector<bool>& preconditionStatus) {
    stringstream ss;
    ss << "SELECTED OPERATOR\n";
    ss << "─────────────────────────────\n";
    ss << "  Name: " << op.name << "\n";
    ss << "  Description: " << op.description << "\n\n";
    
    ss << "  PRECONDITIONS:\n";
    auto preconds = op.getPreconditionDescriptions();
    for (size_t i = 0; i < preconds.size(); ++i) {
        bool status = i < preconditionStatus.size() ? preconditionStatus[i] : false;
        ss << "    [" << (status ? "✓" : "✗") << "] " << preconds[i] << "\n";
    }
    
    ss << "\n  EFFECTS:\n";
    auto effects = op.getEffectDescriptions();
    for (const auto& eff : effects) {
        ss << "    + " << eff << "\n";
    }
    
    ss << "\n";
    return ss.str();
}

string PlannerVisualization::formatAStarEvaluations(const vector<string>& nodeEvals) {
    if (nodeEvals.empty()) return "";
    
    stringstream ss;
    ss << "A* NODE EVALUATIONS (Top 5)\n";
    ss << "─────────────────────────────\n";
    
    int displayed = 0;
    for (const auto& eval : nodeEvals) {
        if (displayed >= 5) break;
        ss << "  " << eval << "\n";
        displayed++;
    }
    
    if (nodeEvals.size() > 5) {
        ss << "  ... (" << (nodeEvals.size() - 5) << " more)\n";
    }
    
    ss << "\n";
    return ss.str();
}

string PlannerVisualization::formatAStarTrace(const SearchTrace& trace) {
    stringstream ss;
    ss << "A* SUCCESSORS\n";
    ss << "─────────────────────────────\n";
    if (trace.immediateSuccessors.empty()) {
        ss << "  No successors evaluated\n\n";
        return ss.str();
    }

    for (const auto& node : trace.immediateSuccessors) {
        ss << "  Node (" << node.pos.x << "," << node.pos.y << "," << node.pos.z << ")"
           << " g=" << node.g << " h=" << node.h << " f=" << node.f;
        if (node.pos == trace.chosenNode) {
            ss << "  [CHOSEN]";
        }
        ss << "\n";
    }

    ss << "\nChosen Node: (" << trace.chosenNode.x << "," << trace.chosenNode.y
       << "," << trace.chosenNode.z << ")\n\n";
    return ss.str();
}

string PlannerVisualization::formatDecisionLog(const PlannerLog::DecisionRecord& record) {
    stringstream ss;
    ss << "TURN " << record.turnNumber << " - " << record.agentType << "\n";
    ss << "═══════════════════════════════════════\n";
    ss << record.finalDecision << "\n";
    ss << "Reasoning: " << record.reasoning << "\n";
    return ss.str();
}

string PlannerVisualization::formatStatusMessage(const Goal& currentGoal,
                                                const string& operator_name,
                                                int stackDepth) {
    stringstream ss;
    ss << "[Goal: " << currentGoal.toString() << "] "
       << "[Op: " << operator_name << "] "
       << "[Stack Depth: " << stackDepth << "]";
    return ss.str();
}

string PlannerVisualization::createFullPanelContent(const vector<Goal>& goalStack,
                                                   const Goal& currentGoal,
                                                   const string& selectedOperator,
                                                   const vector<string>& preconditions,
                                                   const vector<string>& effects,
                                                   const SearchTrace& trace,
                                                   const string& reasoning) {
    stringstream ss;
    ss << "╔════════════════════════════════════════════╗\n";
    ss << "║      EXPLAINABLE AI PLANNING SYSTEM       ║\n";
    ss << "╚════════════════════════════════════════════╝\n\n";
    
    ss << formatGoalStack(goalStack, 5);
    ss << formatCurrentGoal(currentGoal);
    
    if (!selectedOperator.empty()) {
        ss << "SELECTED OPERATOR\n";
        ss << "─────────────────────────────\n";
        ss << "  Name: " << selectedOperator << "\n";
        ss << "\n  PRECONDITIONS:\n";
        for (const auto& pre : preconditions) {
            ss << "    - " << pre << "\n";
        }
        ss << "\n  EFFECTS:\n";
        for (const auto& eff : effects) {
            ss << "    + " << eff << "\n";
        }
        ss << "\n";
    }

    ss << formatAStarTrace(trace);
    
    ss << "REASONING\n";
    ss << "─────────────────────────────\n";
    ss << "  " << reasoning << "\n";
    
    return ss.str();
}

string PlannerVisualization::indent(const string& str, int spaces) {
    stringstream ss;
    string indent_str(spaces, ' ');
    ss << indent_str << str;
    return ss.str();
}

string PlannerVisualization::getGoalTypeSymbol(Goal::Type type) {
    switch (type) {
        case Goal::Type::CATCH: return "🎯";
        case Goal::Type::SAME_CELL: return "⭐";
        case Goal::Type::SAME_FLOOR: return "📍";
        case Goal::Type::REACH_VAULT: return "💰";
        case Goal::Type::ESCAPE: return "🏃";
        case Goal::Type::REACH_EXIT: return "🚪";
        case Goal::Type::MOVE_TO: return "➡️";
        case Goal::Type::AVOID_POLICE: return "⚠️";
        default: return "◇";
    }
}

string PlannerVisualization::formatBool(bool value) {
    return value ? "TRUE" : "FALSE";
}
