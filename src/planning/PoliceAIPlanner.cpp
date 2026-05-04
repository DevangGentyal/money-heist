#include "PoliceAIPlanner.h"
#include <sstream>
#include <climits>
#include <cctype>

// ── helpers ──────────────────────────────────────────────────────────────────
static GoalEntry makeEntry(const string& expr, bool isOp, const string& opName,
                           const vector<string>& pre, const vector<string>& eff) {
    GoalEntry e;
    e.goalExpression = expr;
    e.isOperator     = isOp;
    e.operatorName   = opName;
    e.preconditions  = pre;
    e.effects        = eff;
    e.status         = GoalEntry::PENDING;
    return e;
}

static string normalizePolicePlaceholder(const string& value, const string& policeLabel) {
    string normalized;
    normalized.reserve(value.size() + policeLabel.size());

    for (size_t i = 0; i < value.size();) {
        if (value.compare(i, 6, "Police") == 0) {
            const bool alreadyIndexed = (i + 6 < value.size()) &&
                                        isdigit(static_cast<unsigned char>(value[i + 6]));
            if (!alreadyIndexed) {
                normalized += policeLabel;
                i += 6;
                continue;
            }
        }

        normalized.push_back(value[i]);
        ++i;
    }

    return normalized;
}

// ── ctor ─────────────────────────────────────────────────────────────────────
PoliceAIPlanner::PoliceAIPlanner(int idx, bool hard)
    : policeIndex(idx), hardMode(hard), initialized(false),
      robberOnMyFloor(false), lastVaultStolen(false),
    lastAlertTriggered(false), lastAlertPos(), predictor(nullptr),
      heuristic(nullptr), rules(nullptr) {}

string PoliceAIPlanner::policeLabel() const {
    return "Police" + to_string(policeIndex + 1);
}

// ── factories ─────────────────────────────────────────────────────────────────
GoalEntry PoliceAIPlanner::makeGoal(const string& expr,
                                     const vector<string>& pre,
                                     const vector<string>& eff) {
    return makeEntry(expr, false, "", pre, eff);
}
GoalEntry PoliceAIPlanner::makeOp(const string& expr, const string& opName,
                                   const vector<string>& pre,
                                   const vector<string>& eff) {
    return makeEntry(expr, true, opName, pre, eff);
}

// ── nearest transition cell on a given floor ──────────────────────────────────
Position PoliceAIPlanner::findNearestTransitionCell(const WorldState& world,
                                                     int onFloor) const {
    Position best = world.policePos;
    int bestDist  = INT_MAX;
    if (!world.grid) return best;
    for (int x = 0; x < world.grid->getWidth(); ++x) {
        for (int y = 0; y < world.grid->getHeight(); ++y) {
            Position p(x, y, onFloor);
            CellType ct = world.grid->getCell(p);
            if (ct == CellType::STAIRS) {
                int d = abs(x - world.policePos.x) + abs(y - world.policePos.y);
                if (d < bestDist) { bestDist = d; best = p; }
            }
        }
    }
    return best;
}

// ── block pushers ─────────────────────────────────────────────────────────────
// IMPORTANT: push bottom-first.  back() = top = executes first.

// protect(PoliceN, Vault) — police already on vault floor
// Stack top→bottom after push:
//   sameFloor(PoliceN, Vault)           [precond subgoal]  ← top
//   pathAvailable(PoliceN, Vault)       [precond subgoal]
//   MOVE(PoliceN, Vault)                [operator]
//   protect(PoliceN, Vault)             [goal]  ← bottom
void PoliceAIPlanner::pushProtectVaultBlock(WorldState& world) {
    string pl = policeLabel();

    // Push bottom-first: goal -> operator -> preconditions (right-to-left)
    goalStack.push(makeGoal("protect(" + pl + ", Vault)", {}, {}));
    goalStack.push(makeOp("MOVE(" + pl + ", Vault)", "MOVE",
        {"sameFloor(" + pl + ", Vault)", "pathAvailable(" + pl + ", Vault)"},
        {pl + ".pos=Vault.pos"}));
    goalStack.push(makeGoal("pathAvailable(" + pl + ", Vault)", {}, {}));
    goalStack.push(makeGoal("sameFloor(" + pl + ", Vault)", {}, {}));
}

// protect(PoliceN, Upstairs/Downstairs) — vault on different floor
// Stack top→bottom after push:
//   sameFloor(PoliceN, Upstairs/Downstairs)      [precond subgoal]  ← top
//   pathAvailable(PoliceN, Upstairs/Downstairs)  [precond subgoal]
//   MOVE(PoliceN, Upstairs/Downstairs)           [operator]
//   protect(PoliceN, Upstairs/Downstairs)        [goal]  ← bottom
void PoliceAIPlanner::pushProtectStairsBlock(WorldState& world,
                                              const string& dir) {
    string pl = policeLabel();
    // Avoid duplicate protect goal: only push if not already present
    string protectExpr = "protect(" + pl + ", " + dir + ")";
    // compare ignoring whitespace to avoid false negatives caused by formatting
    auto stripSpaces = [](const string &s) {
        string r; r.reserve(s.size());
        for (char c : s) if (!isspace(static_cast<unsigned char>(c))) r.push_back(c);
        return r;
    };
    auto alphaKey = [](const string &s) {
        string r; r.reserve(s.size());
        for (char c : s) if (isalnum(static_cast<unsigned char>(c))) r.push_back(tolower(static_cast<unsigned char>(c)));
        return r;
    };
    string protectNoWS = stripSpaces(protectExpr);
    string protectKey  = alphaKey(protectNoWS);
    for (const auto &e : goalStack.getStack()) {
        if (alphaKey(stripSpaces(e.goalExpression)) == protectKey) {
            // allow re-pushing only if previous entry is COMPLETED or CANCELLED
            if (e.status != GoalEntry::COMPLETED && e.status != GoalEntry::CANCELLED) return;
            break;
        }
    }

    // Push bottom-first: goal -> operator -> preconditions (right-to-left)
    goalStack.push(makeGoal(protectExpr, {}, {}));
    // Use incremental floor change so each MOVE advances one floor only.
    string floorEffect;
    // MOVE to transition cell only (do NOT change floor until vault stolen)
    goalStack.push(makeOp("MOVE(" + pl + ", " + dir + ")", "MOVE",
        {"sameFloor(" + pl + ", " + dir + ")", "pathAvailable(" + pl + ", " + dir + ")"},
        {}));
    goalStack.push(makeGoal("pathAvailable(" + pl + ", " + dir + ")", {}, {}));
    goalStack.push(makeGoal("sameFloor(" + pl + ", " + dir + ")", {}, {}));
}

// respond(PoliceN, Alert) — pushed ON TOP of existing stack (non-cancelling)
// Stack top→bottom after push:
//   sameFloor(PoliceN, Alert)          [precond subgoal]  ← new top
//   pathAvailable(PoliceN, Alert)      [precond subgoal]
//   MOVE(PoliceN, Alert)               [operator]
//   respond(PoliceN, Alert)            [goal]
//   ── existing protect block stays below ──
void PoliceAIPlanner::pushRespondAlertBlock(WorldState& world) {
    string pl = policeLabel();

    goalStack.push(makeGoal("respond(" + pl + ", Alert)", {}, {}));
    goalStack.push(makeOp("MOVE(" + pl + ", Alert)", "MOVE",
        {"sameFloor(" + pl + ", Alert)", "pathAvailable(" + pl + ", Alert)"},
        {pl + ".pos=Alert.pos"}));
    goalStack.push(makeGoal("pathAvailable(" + pl + ", Alert)", {}, {}));
    goalStack.push(makeGoal("sameFloor(" + pl + ", Alert)", {}, {}));

    // floor hop if needed
    if (world.grid && world.policePos.z != world.alertPos.z) {
        string dir = (world.alertPos.z > world.policePos.z) ? "Upstairs" : "Downstairs";
        // MOVE to transition cell only for respond (do NOT change floor until vault stolen)
        goalStack.push(makeOp("MOVE(" + pl + ", " + dir + ")", "MOVE",
            {"sameFloor(" + pl + ", " + dir + ")", "pathAvailable(" + pl + ", " + dir + ")"},
            {}));
        goalStack.push(makeGoal("pathAvailable(" + pl + ", " + dir + ")", {}, {}));
        goalStack.push(makeGoal("sameFloor(" + pl + ", " + dir + ")", {}, {}));
    }
}

// catch(PoliceN, Robber) — cancel everything first, then push
// Stack top→bottom after push:
//   sameFloor(PoliceN, US/DS)          [precond subgoal]  ← new top (floor-transition if needed)
//   pathAvailable(PoliceN, US/DS)      [precond subgoal]
//   MOVE(PoliceN, US/DS)               [operator floor-transition]
//   sameFloor(PoliceN, Robber)         [precond subgoal]
//   pathAvailable(PoliceN, Robber)     [precond subgoal]
//   MOVE(PoliceN, Robber)              [operator]
//   catch(PoliceN, Robber)             [goal]  ← bottom
void PoliceAIPlanner::pushCatchRobberBlock(WorldState& world) {
    string pl = policeLabel();

    // Push catch block FIRST (will be at bottom)
    goalStack.push(makeGoal("catch(" + pl + ", Robber)", {}, {}));
    goalStack.push(makeOp("MOVE(" + pl + ", Robber)", "MOVE",
        {"sameFloor(" + pl + ", Robber)", "pathAvailable(" + pl + ", Robber)"},
        {pl + ".pos=Robber.pos"}));
    goalStack.push(makeGoal("pathAvailable(" + pl + ", Robber)", {}, {}));
    goalStack.push(makeGoal("sameFloor(" + pl + ", Robber)", {}, {}));

    // Push floor-transition ON TOP (executes first if needed)
    if (world.grid && world.policePos.z != world.robberPos.z) {
        string dir = (world.robberPos.z > world.policePos.z) ? "Upstairs" : "Downstairs";
        // When catching after vault stolen we want an incremental floor-change
        // operator; however, pushCatchRobberBlock is invoked when vault is
        // stolen, so here we push a MOVE that will change floor by one.
        string floorEffect = (world.robberPos.z > world.policePos.z) ? (pl + ".floor+=1") : (pl + ".floor-=1");
        goalStack.push(makeOp("MOVE(" + pl + ", " + dir + ")", "MOVE",
            {"sameFloor(" + pl + ", " + dir + ")", "pathAvailable(" + pl + ", " + dir + ")"},
            {floorEffect}));
        goalStack.push(makeGoal("pathAvailable(" + pl + ", " + dir + ")", {}, {}));
        goalStack.push(makeGoal("sameFloor(" + pl + ", " + dir + ")", {}, {}));
    }
}

// ── init ──────────────────────────────────────────────────────────────────────
void PoliceAIPlanner::initializePlan(const WorldState& world) {
    goalStack.clear();
    string pl = policeLabel();
    // Start halted
    goalStack.push(makeGoal("stayHalt(" + pl + ")", {}, {}));
    robberOnMyFloor    = false;
    initialized        = true;
    lastVaultStolen    = world.vaultStolen;
    lastAlertTriggered = world.alertTriggered;
    lastAlertPos       = world.alertPos;
}

void PoliceAIPlanner::ensureInitialStack(WorldState& world) {
    if (!initialized) initializePlan(world);
}

// ── interrupts ────────────────────────────────────────────────────────────────
void PoliceAIPlanner::handleInterrupts(WorldState& world) {
    string pl = policeLabel();
    bool vaultJustStolen  = world.vaultStolen  && !lastVaultStolen;
    bool alertOnMyFloor   = world.alertTriggered && world.policePos.z == world.alertPos.z;
    bool alertJustFired   = alertOnMyFloor &&
                            (!lastAlertTriggered || !(world.alertPos == lastAlertPos));

    auto hasActiveProtect = [&]() {
        for (const auto &e : goalStack.getStack()) {
            if (e.goalExpression.find("protect(" + pl + ", ") != string::npos &&
                e.status != GoalEntry::COMPLETED && e.status != GoalEntry::CANCELLED) {
                return true;
            }
        }
        return false;
    };

    // ── PRIORITY 1: vault stolen → cancel ALL → push catch ───────────────────
    if (vaultJustStolen || (world.vaultStolen && hasActiveProtect())) {
        goalStack.cancelAll();
        pushCatchRobberBlock(world);
    }
    // ── PRIORITY 2: alert fired → push respond ON TOP (don't cancel protect) ─
    else if (alertJustFired) {
        pushRespondAlertBlock(world);
    }

    // ── Robber first-move-on-MY-floor detection ───────────────────────────────
    if (!robberOnMyFloor && !world.vaultStolen) {
        if (world.robberPos.z == world.policePos.z) {
            robberOnMyFloor = true;
            string pl2 = policeLabel();

            // Remove stayHalt
            bool onHalt = !goalStack.isEmpty() &&
                goalStack.peek().goalExpression == "stayHalt(" + pl2 + ")";
            if (onHalt) {
                goalStack.markComplete();
                goalStack.finalizeCompleted();
            }

            // Vault on this floor?
            if (world.vaultPos.z == world.policePos.z) {
                pushProtectVaultBlock(world);
            } else {
                // Point police toward vault's floor via stairs
                string dir = (world.vaultPos.z > world.policePos.z) ? "Upstairs" : "Downstairs";
                pushProtectStairsBlock(world, dir);
            }
        }
    }

    lastVaultStolen    = world.vaultStolen;
    lastAlertTriggered = world.alertTriggered;
    lastAlertPos       = world.alertPos;
}

void PoliceAIPlanner::onVaultStolen(WorldState& world) {
    // Force immediate cancel and push catch block for this police planner.
    robberOnMyFloor = true;
    goalStack.cancelAll();
    pushCatchRobberBlock(world);
    lastVaultStolen = world.vaultStolen;
}

// ── A* move execution ─────────────────────────────────────────────────────────
Position PoliceAIPlanner::executeMove(const Position& target,
                                       WorldState& world,
                                       GoalType gt) {
    if (!world.grid) return world.policePos;

    world.activeGoalType  = gt;
    world.activeTargetPos = target;

    vector<Position> threat{world.robberPos};
    vector<Position> path;

    if (heuristic && rules) {
        path = AStar3D::findPathWithHeuristic(
            world.policePos, target, *world.grid,
            *heuristic, world, *rules, gt, threat, false,
            world.vaultStolen || world.alertTriggered || world.boostActive, world.stepsTaken);
    } else {
        path = AStar3D::findPath(
            world.policePos, target, *world.grid, gt,
            world.vaultStolen || world.alertTriggered || world.boostActive, world.stepsTaken);
    }

    try {
        lastAstarTrace    = AStar3D::getLastSearchTrace();
        hasLastAstarTrace = true;
    } catch (...) {
        hasLastAstarTrace = false;
    }

    if (path.size() > 1) return path[1];
    return world.policePos;
}

// ── execute top of stack ──────────────────────────────────────────────────────
bool PoliceAIPlanner::executeTop(WorldState& world) {
    if (goalStack.isEmpty()) return false;

    string pl  = policeLabel();
    GoalEntry top = goalStack.peek();

    // ── stayHalt: do nothing ──────────────────────────────────────────────────
    if (top.goalExpression == "stayHalt(" + pl + ")") return false;

    // ── compound precondition (contains '^'): informational only, mark done ───
    if (!top.isOperator && top.goalExpression.find('^') != string::npos) {
        goalStack.markComplete();
        goalStack.finalizeCompleted();
        return true;
    }

    // ── pure goal nodes ───────────────────────────────────────────────────────
        // ── precondition subgoals: check satisfaction and mark COMPLETED ───────
        if (!top.isOperator && !top.goalExpression.empty()) {
            // sameFloor(PoliceN, X): check if police and target on same floor
            if (top.goalExpression.find("sameFloor(") != string::npos) {
                bool satisfied = false;
                if (top.goalExpression.find("Vault") != string::npos)
                    satisfied = (world.policePos.z == world.vaultPos.z);
                else if (top.goalExpression.find("Alert") != string::npos)
                    satisfied = (world.policePos.z == world.alertPos.z);
                else if (top.goalExpression.find("Robber") != string::npos) {
                    satisfied = (world.policePos.z == world.robberPos.z);
                    if (!satisfied && world.vaultStolen) {
                        // Only push floor-transition when vault has been stolen.
                        string dir = (world.robberPos.z > world.policePos.z) ? "Upstairs" : "Downstairs";
                        bool hasPendingMove = false;
                        for (const auto &e : goalStack.getStack()) {
                            if (e.goalExpression.find("MOVE(" + pl + ", " + dir + ")") != string::npos &&
                                e.status != GoalEntry::COMPLETED) { hasPendingMove = true; break; }
                        }
                        if (!hasPendingMove) {
                            // push an incremental MOVE to change one floor toward robber
                            string floorEffect = (world.robberPos.z > world.policePos.z) ? (pl + ".floor+=1") : (pl + ".floor-=1");
                            goalStack.push(makeOp("MOVE(" + pl + ", " + dir + ")", "MOVE",
                                {"sameFloor(" + pl + ", " + dir + ")", "pathAvailable(" + pl + ", " + dir + ")"},
                                {floorEffect}));
                            goalStack.push(makeGoal("pathAvailable(" + pl + ", " + dir + ")", {}, {}));
                            goalStack.push(makeGoal("sameFloor(" + pl + ", " + dir + ")", {}, {}));
                        }
                    }
                }
                else if (top.goalExpression.find("Upstairs") != string::npos ||
                         top.goalExpression.find("Downstairs") != string::npos)
                    satisfied = true;
            
                if (satisfied) {
                    goalStack.markComplete();
                    goalStack.finalizeCompleted();
                    return true;
                } else {
                    goalStack.markPerforming();  // Still working toward this
                    return false;
                }
            }
        
            // pathAvailable(PoliceN, X): assume pathfinding will handle this via A*
            if (top.goalExpression.find("pathAvailable(") != string::npos) {
                // This is checked during MOVE execution, so mark satisfied
                goalStack.markComplete();
                goalStack.finalizeCompleted();
                return true;
            }
        
        }

    if (!top.isOperator) {
        // respond completed: police reached alert pos → protect block naturally surfaces below
        if (top.goalExpression == "respond(" + pl + ", Alert)") {
            if (world.policePos == world.alertPos) {
                goalStack.markComplete();
                goalStack.finalizeCompleted();
                // Old protect block with its operators is already underneath;
                // No need to cancel or re-push. Natural stack unwinding handles it.
                // Ensure that when respond finishes we immediately surface
                // and decompose any underlying protect goal so the police
                // will resume moving toward the protect target.
                bool foundProtect = false;
                for (const auto &e : goalStack.getStack()) {
                    if (e.goalExpression.find("protect(" + pl + ", ") != string::npos &&
                        e.status == GoalEntry::PENDING) {
                        foundProtect = true;
                        // Extract direction (Vault/Upstairs/Downstairs)
                        string dir = e.goalExpression.find("Vault") != string::npos ? "Vault" :
                                    (e.goalExpression.find("Upstairs") != string::npos ? "Upstairs" : "Downstairs");

                        bool hasPendingMove = false;
                        for (const auto &f : goalStack.getStack()) {
                            if (f.goalExpression.find("MOVE(" + pl + ", " + dir + ")") != string::npos &&
                                f.status != GoalEntry::COMPLETED) { hasPendingMove = true; break; }
                        }

                        if (!hasPendingMove) {
                            if (dir == "Vault") {
                                goalStack.push(makeOp("MOVE(" + pl + ", Vault)", "MOVE",
                                    {"sameFloor(" + pl + ", Vault)", "pathAvailable(" + pl + ", Vault)"},
                                    {pl + ".pos=Vault.pos"}));
                            } else {
                                // Protect transitions should move to the transition cell only.
                                // Floor change is chase-only after vault theft.
                                goalStack.push(makeOp("MOVE(" + pl + ", " + dir + ")", "MOVE",
                                    {"sameFloor(" + pl + ", " + dir + ")", "pathAvailable(" + pl + ", " + dir + ")"},
                                    {}));
                            }
                            goalStack.push(makeGoal("pathAvailable(" + pl + ", " + dir + ")", {}, {}));
                            goalStack.push(makeGoal("sameFloor(" + pl + ", " + dir + ")", {}, {}));
                        }
                        break;
                    }
                }
                // If no protect goal found underneath, only rebuild protect
                // while the vault is still uncollected. After theft, keep the
                // stack on chase/catch semantics instead of reintroducing protect.
                if (!foundProtect && !world.vaultStolen) {
                    if (world.vaultPos.z == world.policePos.z) pushProtectVaultBlock(world);
                    else {
                        string dir = (world.vaultPos.z > world.policePos.z) ? "Upstairs" : "Downstairs";
                        pushProtectStairsBlock(world, dir);
                    }
                }
                return true;
            }
            return false;
        }

        if (top.goalExpression == "protect(" + pl + ", Vault)") {
            if (world.vaultStolen) { goalStack.markComplete(); goalStack.finalizeCompleted(); return true; }
            // Keep protect(Vault) PERFORMING indefinitely until interrupted by vault stolen or alert
            if (world.policePos == world.vaultPos) {
                goalStack.markPerforming();  // Actively guarding vault
                return false;
            }
            // Not at vault yet: push MOVE operator + preconditions to reach vault
            bool hasPendingMove = false;
            for (const auto& e : goalStack.getStack()) {
                if (e.goalExpression.find("MOVE(" + pl + ", Vault)") != string::npos &&
                    e.status != GoalEntry::COMPLETED) {
                    hasPendingMove = true;
                    break;
                }
            }
            if (!hasPendingMove) {
                goalStack.push(makeOp("MOVE(" + pl + ", Vault)", "MOVE",
                    {"sameFloor(" + pl + ", Vault)", "pathAvailable(" + pl + ", Vault)"},
                    {pl + ".pos=Vault.pos"}));
                goalStack.push(makeGoal("pathAvailable(" + pl + ", Vault)", {}, {}));
                goalStack.push(makeGoal("sameFloor(" + pl + ", Vault)", {}, {}));
            }
            return false;
        }

        if (top.goalExpression.find("protect(" + pl + ", ") != string::npos) {
            // protect(PoliceN, Upstairs/Downstairs) — reached transition cell?
            Position tc = findNearestTransitionCell(world, world.policePos.z);
            if (world.policePos == tc || world.vaultStolen) {
                goalStack.markComplete();
                goalStack.finalizeCompleted();
                return true;
            }
            // Not satisfied yet: push the MOVE operator + preconditions to achieve this goal
            // Extract direction from protect goal expression
            string dir = top.goalExpression.find("Vault") != string::npos ? "Vault" :
                        (top.goalExpression.find("Upstairs") != string::npos ? "Upstairs" : "Downstairs");
            
            // Only push if not already being pursued (check for pending/active MOVE)
            bool hasPendingMove = false;
            for (const auto& e : goalStack.getStack()) {
                if (e.goalExpression.find("MOVE(" + pl + ", " + dir + ")") != string::npos &&
                    e.status != GoalEntry::COMPLETED) {
                    hasPendingMove = true;
                    break;
                }
            }
            
            if (!hasPendingMove) {
                // Push decomposition: preconditions → operator
                if (dir == "Vault") {
                    goalStack.push(makeOp("MOVE(" + pl + ", Vault)", "MOVE",
                        {"sameFloor(" + pl + ", Vault)", "pathAvailable(" + pl + ", Vault)"},
                        {pl + ".pos=Vault.pos"}));
                } else {
                    goalStack.push(makeOp("MOVE(" + pl + ", " + dir + ")", "MOVE",
                        {"sameFloor(" + pl + ", " + dir + ")", "pathAvailable(" + pl + ", " + dir + ")"},
                        {}));
                }
                goalStack.push(makeGoal("pathAvailable(" + pl + ", " + dir + ")", {}, {}));
                goalStack.push(makeGoal("sameFloor(" + pl + ", " + dir + ")", {}, {}));
            }
            return false;
        }

        if (top.goalExpression == "catch(" + pl + ", Robber)") {
            if (world.policePos == world.robberPos) {
                world.policeWon = true;
                goalStack.markComplete();
                goalStack.finalizeCompleted();
                return true;
            }
            return false;
        }

        return false;
    }

    // ── operator nodes ────────────────────────────────────────────────────────
    if (top.operatorName == "MOVE") {
        Position target = world.vaultPos;
        GoalType gt     = GoalType::PROTECT_VAULT;

        if (top.goalExpression.find("Robber") != string::npos) {
            target = world.robberPos;   // re-evaluated every turn
            gt     = GoalType::CHASE_ROBBER;
        } else if (top.goalExpression.find("Alert") != string::npos) {
            target = world.alertPos;
            gt     = GoalType::RESPOND_ALERT;
        } else if (top.goalExpression.find("Upstairs") != string::npos ||
                   top.goalExpression.find("Downstairs") != string::npos) {
            target = findNearestTransitionCell(world, world.policePos.z);
            gt     = GoalType::MOVE_FLOOR_GOAL;
        } else if (top.goalExpression.find("Vault") != string::npos) {
            target = world.vaultPos;
            gt     = GoalType::PROTECT_VAULT;
        }

        world.activeGoalType = gt;
        Position next = executeMove(target, world, gt);
        world.policePos = next;

        if (next == target) {
            // If this MOVE operator has an explicit incremental floor effect,
            // apply it immediately. Otherwise, fall back to the previous rule which
            // allows automatic transition when the vault is stolen.
            bool appliedFloor = false;
            for (const auto &eff : top.effects) {
                auto posInc = eff.find(".floor+=");
                auto posDec = eff.find(".floor-=");
                if (posInc != string::npos) {
                    string num = eff.substr(posInc + 7);
                    int delta = 1;
                    try { delta = stoi(num); } catch (...) { delta = 1; }
                    if (world.grid) {
                        int newf = world.policePos.z + delta;
                        if (newf < 0) newf = 0;
                        if (newf >= world.grid->getDepth()) newf = world.grid->getDepth() - 1;
                        world.policePos.z = newf;
                    } else {
                        world.policePos.z += delta;
                    }
                    appliedFloor = true;
                    break;
                } else if (posDec != string::npos) {
                    string num = eff.substr(posDec + 7);
                    int delta = 1;
                    try { delta = stoi(num); } catch (...) { delta = 1; }
                    if (world.grid) {
                        int newf = world.policePos.z - delta;
                        if (newf < 0) newf = 0;
                        if (newf >= world.grid->getDepth()) newf = world.grid->getDepth() - 1;
                        world.policePos.z = newf;
                    } else {
                        world.policePos.z -= delta;
                    }
                    appliedFloor = true;
                    break;
                }
            }

            if (!appliedFloor) {
                // Check if reached stairs to auto-transition floors
                // Only allow the automatic step when the vault is stolen
                if (world.vaultStolen && world.grid) {
                    CellType cellType = world.grid->getCell(next);
                    if (cellType == CellType::STAIRS) {
                        if (top.goalExpression.find("Downstairs") != string::npos) {
                            if (next.z > 0) world.policePos.z--;  // Move DOWN one floor
                        } else {
                            if (next.z < world.grid->getDepth() - 1) world.policePos.z++;  // Move UP one floor
                        }
                    }
                }
            }

            goalStack.markComplete();
            goalStack.finalizeCompleted();
        } else {
            goalStack.markPerforming();
        }
        return true;
    }

    if (top.operatorName == "MOVE_FLOOR") {
        goalStack.markComplete();
        goalStack.finalizeCompleted();
        return true;
    }

    if (top.operatorName == "CAPTURE") {
        if (world.policePos == world.robberPos) {
            world.policeWon = true;
            goalStack.markComplete();
            goalStack.finalizeCompleted();
        }
        return true;
    }

    return false;
}

void PoliceAIPlanner::pushClearVaultGoal(WorldState& world) {
    if (world.policePos != world.vaultPos) return;

    auto isClearGoalPresent = [&]() {
        for (const auto& entry : goalStack.getStack()) {
            if (entry.goalExpression == "clear(Vault)" &&
                entry.status != GoalEntry::COMPLETED &&
                entry.status != GoalEntry::CANCELLED) {
                return true;
            }
        }
        return false;
    };

    if (isClearGoalPresent()) return;

    goalStack.removeGoalFromCompleted("clear(Vault)");
    GoalEntry clearGoal;
    clearGoal.goalExpression = "clear(Vault)";
    clearGoal.isOperator = false;
    goalStack.push(clearGoal);
}

// ── main turn ─────────────────────────────────────────────────────────────────
Position PoliceAIPlanner::runTurn(WorldState& world) {
    ensureInitialStack(world);
    // Ensure any previously completed entries are removed first
    goalStack.finalizeCompleted();
    handleInterrupts(world);
    pushClearVaultGoal(world);

    // Keep executing the top entry and finalizing completed entries
    // until no more entries get popped. This ensures that when
    // preconditions become satisfied they are drained in the same
    // turn and the operator can execute immediately.
    while (true) {
        size_t before = goalStack.getStack().size();
        // attempt to act on the top entry (may markComplete, perform, or execute an operator)
        executeTop(world);
        // remove any completed/cancelled entries
        goalStack.finalizeCompleted();
        size_t after = goalStack.getStack().size();

        // If something was popped, loop again to process the new top.
        if (after < before) continue;

        // Nothing popped this iteration — stop so we don't perform multiple
        // in-progress operations in a single turn.
        break;
    }

    return world.policePos;
}

// ── external goal push ────────────────────────────────────────────────────────
void PoliceAIPlanner::pushExternalGoal(const string& expr,
                                        const vector<string>& pre,
                                        const vector<string>& eff,
                                        bool isOperator) {
    const string pl = policeLabel();
    const string normalizedExpr = normalizePolicePlaceholder(expr, pl);

    vector<string> normalizedPre;
    normalizedPre.reserve(pre.size());
    for (const auto& item : pre) {
        normalizedPre.push_back(normalizePolicePlaceholder(item, pl));
    }

    vector<string> normalizedEff;
    normalizedEff.reserve(eff.size());
    for (const auto& item : eff) {
        normalizedEff.push_back(normalizePolicePlaceholder(item, pl));
    }

    // Prevent duplicate external pushes: if the same normalized expression
    // already exists on the stack and is not COMPLETED, skip pushing.
    auto stripSpaces = [](const string &s) {
        string r; r.reserve(s.size());
        for (char c : s) if (!isspace(static_cast<unsigned char>(c))) r.push_back(c);
        return r;
    };
    auto alphaKey = [](const string &s) {
        string r; r.reserve(s.size());
        for (char c : s) if (isalnum(static_cast<unsigned char>(c))) r.push_back(tolower(static_cast<unsigned char>(c)));
        return r;
    };
    string normNoWS = stripSpaces(normalizedExpr);
    string normKey  = alphaKey(normNoWS);
    for (const auto &e : goalStack.getStack()) {
        if (alphaKey(stripSpaces(e.goalExpression)) == normKey) {
            if (e.status != GoalEntry::COMPLETED && e.status != GoalEntry::CANCELLED) return; // already present and active
            break; // previous completed or cancelled; allow push
        }
    }

    if (isOperator) goalStack.push(makeOp(normalizedExpr, normalizedExpr, normalizedPre, normalizedEff));
    else            goalStack.push(makeGoal(normalizedExpr, normalizedPre, normalizedEff));
}


string PoliceAIPlanner::getPlanningDashboard() const {
    return goalStack.debugSummary();
}