// PoliceAI.cpp
#include "PoliceAI.h"

PoliceAI::PoliceAI(const Position& startPos, int policeIndex,
                   PredictionEngine* pred,
                   HeuristicEngine* h,
                   RuleEngine* r,
                   bool hard)
    : Agent(startPos, AgentRole::POLICE),
      state(PoliceState::PATROL),
      predictor(pred),
      heuristic(h),
      rules(r),
      vaultCollected(false),
      hardMode(hard) {
    planner = make_unique<PoliceAIPlanner>(policeIndex, hardMode);
    planner->setPredictionEngine(pred);
    planner->setHeuristicEngine(h);
    planner->setRuleEngine(r);
}

Position PoliceAI::getNextMove(const Grid3D& grid,
                              const vector<Position>& otherAgents) {
    if (otherAgents.empty() || !planner) {
        return pos;
    }

    WorldState world;
    world.grid = &grid;
    world.policePos = pos;
    world.robberPos = otherAgents[0];
    world.vaultPos = grid.getVaultPos();
    world.exitPos = grid.getExitPos();
    world.alertPos = rules ? rules->getAlertPos() : Position();
    world.robberHasVault = vaultCollected;
    world.vaultStolen = rules ? rules->isVaultStolen() : vaultCollected;
    world.alertTriggered = rules ? rules->isAlertActive() : false;
    world.boostActive = rules ? rules->isBoostActive() : false;
    world.policePositions = {pos};
    world.stepsTaken = getStepsTaken();  // Pass current step count for A* initialG

    planner->setHardMode(hardMode);
    Position nextMove = planner->runTurn(world);
    // Commit the move — this increments stepsTaken
    commitMove(nextMove);  
    pos = nextMove;
    planningDashboard = planner->getPlanningDashboard();
    return nextMove;
}

void PoliceAI::updateState(const Grid3D&, const Position&, bool) {}

Position PoliceAI::computeInterceptPath(const Grid3D&, const Position&) { return pos; }

Position PoliceAI::predictTargetPosition(const Grid3D&, const Position& targetPos) { return targetPos; }
