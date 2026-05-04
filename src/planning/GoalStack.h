#pragma once
#include <memory>
#include <string>
#include <vector>

namespace ai {
namespace strips {
struct Operator;
} }

using namespace std;

struct GoalEntry {
    enum Status { PENDING, ACTIVE, PERFORMING, COMPLETED, CANCELLED };
    string goalExpression;
    string operatorName;
    bool   isOperator = false;
    Status status     = PENDING;
    vector<string> preconditions;
    vector<string> effects;
    shared_ptr<ai::strips::Operator> operatorData;
};

class GoalStack {
public:
    GoalStack();

    void      push(const GoalEntry& goal);   // top = back()
    GoalEntry pop();
    GoalEntry peek() const;
    bool      isEmpty() const;

    void cancel(const string& goalExpression);
    void cancelAll();
    void markComplete();
    void markPerforming();
    void finalizeCompleted();       // remove COMPLETED/CANCELLED from top
    void removeGoalFromCompleted(const string& goalExpression);  // Remove specific goal from completed list

    // Returns top-first (index 0 = top)
    vector<GoalEntry> getStack()          const;
    vector<GoalEntry> getCompletedGoals() const;
    vector<GoalEntry> getCancelledGoals() const;

    void   clear();
    string debugSummary() const;

private:
    vector<GoalEntry> stackEntries;   // back() = top
    vector<GoalEntry> completedGoals;
    vector<GoalEntry> cancelledGoals;

    void promoteTopToActive();
};