#include "GoalStack.h"
#include <sstream>

GoalStack::GoalStack() = default;

void GoalStack::promoteTopToActive() {
    for (auto it = stackEntries.rbegin(); it != stackEntries.rend(); ++it) {
        if (it->status != GoalEntry::COMPLETED &&
            it->status != GoalEntry::CANCELLED) {
            it->status = GoalEntry::ACTIVE;
            return;
        }
    }
}

void GoalStack::push(const GoalEntry& goal) {
    // demote current top
    for (auto& e : stackEntries) {
        if (e.status == GoalEntry::ACTIVE ||
            e.status == GoalEntry::PERFORMING)
            e.status = GoalEntry::PENDING;
    }
    GoalEntry e = goal;
    e.status = GoalEntry::ACTIVE;
    stackEntries.push_back(e);
}

GoalEntry GoalStack::pop() {
    if (stackEntries.empty()) return GoalEntry();
    GoalEntry e = stackEntries.back();
    stackEntries.pop_back();
    promoteTopToActive();
    return e;
}

GoalEntry GoalStack::peek() const {
    if (stackEntries.empty()) return GoalEntry();
    return stackEntries.back();
}

bool GoalStack::isEmpty() const { return stackEntries.empty(); }

void GoalStack::cancel(const string& expr) {
    for (auto it = stackEntries.rbegin(); it != stackEntries.rend(); ++it) {
        if (it->goalExpression == expr &&
            it->status != GoalEntry::COMPLETED &&
            it->status != GoalEntry::CANCELLED) {
            it->status = GoalEntry::CANCELLED;
            cancelledGoals.push_back(*it);
            promoteTopToActive();
            return;
        }
    }
}

void GoalStack::cancelAll() {
    for (auto& e : stackEntries) {
        if (e.status != GoalEntry::COMPLETED &&
            e.status != GoalEntry::CANCELLED) {
            e.status = GoalEntry::CANCELLED;
            cancelledGoals.push_back(e);
        }
    }
    stackEntries.clear();
}

void GoalStack::markComplete() {
    if (stackEntries.empty()) return;
    GoalEntry& top = stackEntries.back();
    if (top.status == GoalEntry::COMPLETED) return;
    top.status = GoalEntry::COMPLETED;
    completedGoals.push_back(top);
}

void GoalStack::markPerforming() {
    if (!stackEntries.empty())
        stackEntries.back().status = GoalEntry::PERFORMING;
}

void GoalStack::finalizeCompleted() {
    while (!stackEntries.empty() &&
           (stackEntries.back().status == GoalEntry::COMPLETED ||
            stackEntries.back().status == GoalEntry::CANCELLED))
        stackEntries.pop_back();
    promoteTopToActive();
}

vector<GoalEntry> GoalStack::getStack() const {
    vector<GoalEntry> r;
    for (auto it = stackEntries.rbegin(); it != stackEntries.rend(); ++it)
        r.push_back(*it);
    return r;
}

vector<GoalEntry> GoalStack::getCompletedGoals() const { return completedGoals; }
vector<GoalEntry> GoalStack::getCancelledGoals()  const { return cancelledGoals; }

void GoalStack::clear() {
    stackEntries.clear();
    completedGoals.clear();
    cancelledGoals.clear();
}

string GoalStack::debugSummary() const {
    stringstream ss;
    ss << "STACK=" << stackEntries.size()
       << " DONE=" << completedGoals.size()
       << " CANCELLED=" << cancelledGoals.size();
    if (!stackEntries.empty())
        ss << " TOP=[" << stackEntries.back().goalExpression << "]";
    return ss.str();
}