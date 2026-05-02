#include "PlannerLog.h"
#include <iomanip>

void PlannerLog::recordDecision(const DecisionRecord& record) {
    records.push_back(record);
}

const PlannerLog::DecisionRecord* PlannerLog::getLatestRecord() const {
    if (records.empty()) return nullptr;
    return &records.back();
}

vector<PlannerLog::DecisionRecord> PlannerLog::getRecordsForTurn(int turn) const {
    vector<DecisionRecord> result;
    for (const auto& rec : records) {
        if (rec.turnNumber == turn) {
            result.push_back(rec);
        }
    }
    return result;
}

string PlannerLog::exportToString() const {
    stringstream ss;
    ss << "=== PLANNER LOG ===" << endl;
    ss << "Total decisions: " << records.size() << endl << endl;
    
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& rec = records[i];
        ss << "--- Decision " << (i + 1) << " ---" << endl;
        ss << formatDecisionForDisplay(rec) << endl;
    }
    
    return ss.str();
}

string PlannerLog::exportAsCSV() const {
    stringstream ss;
    // CSV Header
    ss << "Turn,Agent,Pos,Goal,Operator,Decision,Reasoning" << endl;
    
    for (const auto& rec : records) {
        ss << rec.turnNumber << ","
           << rec.agentType << ","
           << formatPosition(rec.agentPos) << ","
           << rec.currentGoal << ","
           << rec.selectedOperator << ","
           << rec.finalDecision << ","
           << rec.reasoning << endl;
    }
    
    return ss.str();
}

string PlannerLog::formatDecisionForDisplay(const DecisionRecord& rec) const {
    stringstream ss;
    ss << "Turn: " << rec.turnNumber << endl;
    ss << "Agent: " << rec.agentType << " at " << formatPosition(rec.agentPos) << endl;
    ss << "Target: " << formatPosition(rec.targetPos) << endl;
    ss << endl;
    
    ss << "GOAL STACK (top 3):" << endl;
    for (const auto& goal : rec.goalStackTop3) {
        ss << "  > " << goal << endl;
    }
    ss << endl;
    
    ss << "CURRENT GOAL: " << rec.currentGoal << endl;
    ss << endl;
    
    ss << "SELECTED OPERATOR: " << rec.selectedOperator << endl;
    ss << formatPreconditionStatus(rec.preconditions, rec.preconditionStatus);
    ss << endl;
    
    ss << "EFFECTS:" << endl;
    for (const auto& eff : rec.effects) {
        ss << "  + " << eff << endl;
    }
    ss << endl;
    
    if (!rec.astarNodeEvaluations.empty()) {
        ss << "A* NODE EVALUATIONS:" << endl;
        for (const auto& eval : rec.astarNodeEvaluations) {
            ss << "  " << eval << endl;
        }
        ss << endl;
    }
    
    ss << "FINAL DECISION: " << rec.finalDecision << endl;
    ss << "REASONING: " << rec.reasoning << endl;
    
    return ss.str();
}

string PlannerLog::formatPosition(const Position& p) const {
    stringstream ss;
    ss << "(" << p.x << "," << p.y << "," << p.z << ")";
    return ss.str();
}

string PlannerLog::formatPreconditionStatus(const vector<string>& preconds,
                                           const vector<bool>& status) const {
    stringstream ss;
    ss << "PRECONDITIONS:" << endl;
    for (size_t i = 0; i < preconds.size(); ++i) {
        ss << "  [" << (i < status.size() && status[i] ? "✓" : "✗") << "] "
           << preconds[i] << endl;
    }
    return ss.str();
}
