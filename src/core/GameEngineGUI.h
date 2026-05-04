#ifndef GAMEENGINE_GUI_H
#define GAMEENGINE_GUI_H

#include "../grid/Grid3D.h"
#include "../agents/RobberAI.h"
#include "../agents/PoliceAI.h"
#include "../ai/AStar3D.h"
#include "../ai/HeuristicEngine.h"
#include "../ai/PredictionEngine.h"
#include "../rules/RuleEngine.h"
#include "../rendering/RaylibRenderer.h"
#include <vector>
#include <memory>

#ifdef __APPLE__
#include <sys/types.h>
#endif

using namespace std;

enum class PlayerRole
{
    ROBBER,
    POLICE
};

enum class GameStatus
{
    RUNNING,
    ROBBER_WON,
    ROBBER_ESCAPED_NO_VAULT,
    POLICE_WON,
    QUIT
};

class GameEngineGUI
{
private:
    static constexpr int gridStartX = 20;
    static constexpr int gridStartY = 20;

    Grid3D grid;
    vector<unique_ptr<RobberAI>> robbers;
    vector<unique_ptr<PoliceAI>> policeList;
    int controlledPoliceIndex;
    unique_ptr<HeuristicEngine> heuristic;
    unique_ptr<PredictionEngine> predictor;
    unique_ptr<RuleEngine> rules;
    unique_ptr<RaylibRenderer> renderer;

    PlayerRole playerRole;
    int difficulty;
    int turnCount;
    GameStatus gameStatus;
    string statusMessage;
    bool vaultCollected;
    int vaultPopupFrames;
    string popupMessage;
    int policeBoostTurnsRemaining;
    bool policeDoubleStepActive;
    pid_t debugWindowPid;
    string debugSnapshotPath;
    vector<string> debugSnapshotPaths;
    vector<string> robberDebugSnapshotPaths;
    vector<pid_t> robberDebugWindowPids;
    // track robber last positions to detect first-move-on-floor events
    Position lastRobberPos;
    vector<pid_t> debugWindowPids;
    // Alert teleportation state (only for police user mode)
    bool alertTriggered;
    Position alertTeleportPos;
    int alertMovesLeft;

    // Input tracking
    Position hoveredCell;
    bool hasHoveredCell;

public:
    GameEngineGUI();
    ~GameEngineGUI();

    // Game flow
    void initialize(int diff, PlayerRole role);
    void run();
    void handleInput();
    void update();
    void render();
    void checkWinConditions();
    void updateAI();
    bool processRobberFirstTurnInPoliceMode();
    string buildDebugSnapshot() const;
    string buildDebugSnapshotForPolice(int idx) const;
    string buildDebugSnapshotForRobber(int idx) const;
    void spawnDebugWindow();
    void stopDebugWindow();
    void spawnDebugWindowForRobber(int index, const string &path);

    bool tryMovePlayer(const Position &targetPos);
    bool isControllableMoveValid(const Position &targetPos) const;
    bool arePoliceFloorChangesAllowed() const;
    bool shouldPoliceTransitionFloors(const Position &policePos) const;
    void syncControlledPoliceToRobberFloor();
    Position getMouseGridPosition() const;
    Position getPlayerPosition() const;
    void setPlayerPosition(const Position &newPos);
    bool isPoliceOnCell(const Position &pos) const;
    void refreshVaultCollectionState();
    void spawnDebugWindowForPolice(int index, const string &path);

    // Getters
    GameStatus getGameStatus() const { return gameStatus; }
    string getStatusMessage() const { return statusMessage; }
};

#endif
