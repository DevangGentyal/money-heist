#ifndef STRIPS_OPERATORS_H
#define STRIPS_OPERATORS_H

#include <functional>
#include <string>
#include <vector>

#include "../grid/Grid3D.h"

using namespace std;

using Cell = Position;

enum class GoalType {
    PROTECT_VAULT,
    CHASE_ROBBER,
    RESPOND_ALERT,
    REACH_VAULT,
    ESCAPE_TO_EXIT,
    REACH_ALERT_ZONE,
    MOVE_FLOOR_GOAL,
    NONE
};

struct WorldState {
    const Grid3D* grid = nullptr;
    Position robberPos;
    Position policePos;
    Position vaultPos;
    Position exitPos;
    Position alertPos;
    vector<Position> policePositions;
    bool robberHasVault = false;
    bool vaultStolen = false;
    bool alertTriggered = false;
    bool boostActive = false;
    bool robberWon = false;
    bool policeWon = false;
    bool robberOnVault = false;
    bool robberOnExit = false;
    bool policeOnVault = false;
    bool policeOnAlert = false;
    GoalType activeGoalType = GoalType::NONE;
    Position activeTargetPos;
    int activeTargetFloor = 0;
    int stepsTaken = 0;  // Current agent step count for A* initialG

    bool hasGrid() const { return grid != nullptr; }
};

struct STRIPSOperator {
    string name;
    vector<string> preconditions;
    vector<string> effects;
    function<bool(WorldState&)> checkPreconditions;
    function<void(WorldState&)> applyEffects;
};

inline string positionToString(const Position& pos) {
    return "[" + to_string(pos.x) + "," + to_string(pos.y) + "," + to_string(pos.z) + "]";
}

inline STRIPSOperator makeMoveOperator(const string& agentName, const Position& target) {
    STRIPSOperator op;
    op.name = "MOVE";
    op.preconditions = {
        "pathExists(" + agentName + "," + positionToString(target) + ")",
        "NOT wall(" + positionToString(target) + ")"
    };
    op.effects = {
        agentName + ".pos=" + positionToString(target)
    };
    op.checkPreconditions = [target, agentName](WorldState& w) {
        if (!w.grid) return false;
        Position from = agentName == "Police" ? w.policePos : w.robberPos;
        if (!w.grid->isValid(target) || w.grid->isWall(target)) return false;
        return true;
    };
    op.applyEffects = [target, agentName](WorldState& w) {
        if (agentName == "Police") {
            w.policePos = target;
        } else {
            w.robberPos = target;
        }
    };
    return op;
}

inline STRIPSOperator makeMoveFloorOperator(const string& agentName, int targetFloor) {
    STRIPSOperator op;
    op.name = "MOVE_FLOOR";
    op.preconditions = {
        "transitionCellExists",
        "floorTransitionAllowed"
    };
    op.effects = {
        agentName + ".floor=" + to_string(targetFloor)
    };
    op.checkPreconditions = [targetFloor, agentName](WorldState& w) {
        if (!w.grid) return false;
        const Position current = agentName == "Police" ? w.policePos : w.robberPos;
        if (current.z == targetFloor) return true;
        CellType currentType = w.grid->getCell(current);
        if (currentType != CellType::STAIRS && currentType != CellType::ELEVATOR) return false;
        if (agentName == "Police" && !(w.vaultStolen || w.alertTriggered || w.boostActive)) return false;
        return true;
    };
    op.applyEffects = [targetFloor, agentName](WorldState& w) {
        if (agentName == "Police") {
            w.policePos.z = targetFloor;
        } else {
            w.robberPos.z = targetFloor;
        }
    };
    return op;
}

inline STRIPSOperator makeStealOperator() {
    STRIPSOperator op;
    op.name = "STEAL";
    op.preconditions = {
        "Robber.pos == Vault.pos",
        "NOT vaultStolen"
    };
    op.effects = {
        "vaultStolen=true",
        "Robber.hasVault=true"
    };
    op.checkPreconditions = [](WorldState& w) {
        return w.robberPos == w.vaultPos && !w.vaultStolen;
    };
    op.applyEffects = [](WorldState& w) {
        w.vaultStolen = true;
        w.robberHasVault = true;
    };
    return op;
}

inline STRIPSOperator makeEscapeOperator() {
    STRIPSOperator op;
    op.name = "ESCAPE";
    op.preconditions = {
        "Robber.pos == Exit.pos",
        "Robber.hasVault == true"
    };
    op.effects = {
        "RobberWins=true"
    };
    op.checkPreconditions = [](WorldState& w) {
        return w.robberPos == w.exitPos && w.robberHasVault;
    };
    op.applyEffects = [](WorldState& w) {
        w.robberWon = true;
    };
    return op;
}

inline STRIPSOperator makeCaptureOperator() {
    STRIPSOperator op;
    op.name = "CAPTURE";
    op.preconditions = {
        "Police.pos == Robber.pos"
    };
    op.effects = {
        "PoliceWins=true"
    };
    op.checkPreconditions = [](WorldState& w) {
        return w.policePos == w.robberPos;
    };
    op.applyEffects = [](WorldState& w) {
        w.policeWon = true;
    };
    return op;
}

inline STRIPSOperator makeDistractOperator(const Position& alertZonePos) {
    STRIPSOperator op;
    op.name = "DISTRACT";
    op.preconditions = {
        "alertZoneExists"
    };
    op.effects = {
        "alertTriggered=true",
        "alertPos=" + positionToString(alertZonePos)
    };
    op.checkPreconditions = [alertZonePos](WorldState& w) {
        if (!w.grid) return false;
        return w.grid->isAlertZone(alertZonePos);
    };
    op.applyEffects = [alertZonePos](WorldState& w) {
        w.alertTriggered = true;
        w.alertPos = alertZonePos;
    };
    return op;
}

inline STRIPSOperator makeRespondAlertOperator() {
    STRIPSOperator op;
    op.name = "RESPOND_ALERT";
    op.preconditions = {
        "alertTriggered == true"
    };
    op.effects = {
        "Police.respondingToAlert=true"
    };
    op.checkPreconditions = [](WorldState& w) {
        return w.alertTriggered;
    };
    op.applyEffects = [](WorldState& w) {
        w.boostActive = true;
    };
    return op;
}

inline STRIPSOperator makeStayHaltOperator() {
    STRIPSOperator op;
    op.name = "STAY_HALT";
    op.preconditions = {"always true"};
    op.effects = {"nothing"};
    op.checkPreconditions = [](WorldState&) { return true; };
    op.applyEffects = [](WorldState&) {};
    return op;
}

#endif