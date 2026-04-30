#ifndef ROBBER_AI_H
#define ROBBER_AI_H

#include "Agent.h"
#include "../ai/AStar3D.h"
#include "../ai/HeuristicEngine.h"

using namespace std;

enum class RobberState {
    HUNTING_VAULT,
    ESCAPING,
    EVASION,
    CORNERED
};

class RobberAI : public Agent {
private:
    Position targetGoal;
    RobberState state;
    bool hasVault;
    HeuristicEngine* heuristic;
    int turnsWaiting;
    Position lastPos;
    
public:
    RobberAI(const Position& startPos, HeuristicEngine* h = nullptr);
    
    Position getNextMove(const Grid3D& grid, 
                        const vector<Position>& policePositions) override;
    
    // Strategic functions
    void updateState(const Grid3D& grid, const vector<Position>& policePositions);
    Position computeSafePath(const Grid3D& grid, 
                           const vector<Position>& policePositions);
    float evaluateDanger(const Position& p, const vector<Position>& policePositions) const;
    
    // Getters/Setters
    RobberState getState() const { return state; }
    bool hasReachedVault(const Grid3D& grid) const;
    bool hasReachedExit(const Grid3D& grid) const;
    bool hasCollectedVault() const { return hasVault; }
    void setHeuristic(HeuristicEngine* h) { heuristic = h; }
};

#endif
