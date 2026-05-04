#pragma once
#include "../planning/GoalStack.h"
// #include "../world/WorldState.h"
#include "../ai/AStar3D.h"
#include "../ai/HeuristicEngine.h"
#include "../ai/PredictionEngine.h"
#include "../rules/RuleEngine.h"
#include <string>
#include <vector>
using namespace std;

class PoliceAIPlanner {
public:
    explicit PoliceAIPlanner(int policeIndex, bool hardMode = false);

    // Dependencies
    void setPredictionEngine(PredictionEngine* p) { predictor = p; }
    void setHeuristicEngine (HeuristicEngine*  h) { heuristic = h; }
    void setRuleEngine      (RuleEngine*        r) { rules = r;     }
    void setHardMode        (bool h)               { hardMode = h;  }

    // Main turn
    Position runTurn(WorldState& world);

    // External push (alert/vault events from GameEngineGUI)
    void pushExternalGoal(const string& expression,
                          const vector<string>& preconditions,
                          const vector<string>& effects,
                          bool isOperator = false);

    // Immediate vault-stolen notification: cancel and push catch block
    void onVaultStolen(WorldState& world);

    // Debug
    const GoalStack& getGoalStack() const { return goalStack; }
    GoalStack&       getGoalStack()       { return goalStack; }
    string           getPlanningDashboard() const;
    bool             hasAstarTrace()  const { return hasLastAstarTrace; }
    SearchTrace      getLastAstarTrace() const { return lastAstarTrace; }

private:
    int  policeIndex;
    bool hardMode;
    bool initialized;
    bool robberOnMyFloor;      // has robber stepped on MY floor?
    bool lastVaultStolen;
    bool lastAlertTriggered;
    Position lastAlertPos;

    GoalStack        goalStack;
    PredictionEngine* predictor;
    HeuristicEngine*  heuristic;
    RuleEngine*       rules;

    SearchTrace lastAstarTrace;
    bool        hasLastAstarTrace = false;

    // Entry factories
    GoalEntry makeGoal(const string& expr,
                       const vector<string>& pre = {},
                       const vector<string>& eff = {});
    GoalEntry makeOp  (const string& expr, const string& opName,
                       const vector<string>& pre = {},
                       const vector<string>& eff = {});

    // Block pushers  (push bottom-first so top lands correctly)
    void pushProtectVaultBlock  (WorldState& world);
    void pushProtectStairsBlock (WorldState& world, const string& dir);
    void pushRespondAlertBlock  (WorldState& world);
    void pushCatchRobberBlock   (WorldState& world);

    // Logic
    void initializePlan   (const WorldState& world);
    void ensureInitialStack(WorldState& world);
    void handleInterrupts (WorldState& world);
    bool executeTop       (WorldState& world);
    Position executeMove  (const Position& target, WorldState& world, GoalType gt);
    void pushClearVaultGoal(WorldState& world);

    // Helpers
    Position findNearestTransitionCell(const WorldState& world, int onFloor) const;
    string   policeLabel() const;   // "Police1", "Police2", …
};