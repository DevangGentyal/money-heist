// GameEngineGUI.cpp
#include "GameEngineGUI.h"
#include "../planning/STRIPSOperators.h"
#include "raylib.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <fstream>
#include <cctype>
#include <filesystem>
#include <thread>
#include <chrono>
#ifdef __APPLE__
#include <unistd.h>
#include <signal.h>
#endif

namespace {
void applyRobberFloorTransition(RobberAI &robber, const Grid3D &grid) {
  Position current = robber.getPosition();
  CellType atCell = grid.getCell(current);

  if (atCell == CellType::STAIRS) {
    Position up(current.x, current.y, current.z + 1);
    if (grid.isValid(up) && !grid.isWall(up)) {
      robber.setPosition(up);
    }
  } else if (atCell == CellType::ELEVATOR) {
    // DS tile: move one floor down.
    Position down(current.x, current.y, current.z - 1);
    if (grid.isValid(down) && !grid.isWall(down)) {
      robber.setPosition(down);
    }
  }
}

string policeStateName(PoliceState state) {
  switch (state) {
    case PoliceState::PATROL:
      return "PATROL";
    case PoliceState::ALERT:
      return "ALERT";
    case PoliceState::CHASE:
      return "CHASE";
    case PoliceState::INTERCEPT:
      return "INTERCEPT";
  }
  return "PATROL";
}

string robberStateName(RobberState state) {
  switch (state) {
    case RobberState::HUNTING_VAULT:
      return "HUNTING_VAULT";
    case RobberState::ESCAPING:
      return "ESCAPING";
    case RobberState::EVASION:
      return "EVASION";
    case RobberState::CORNERED:
      return "CORNERED";
  }
  return "HUNTING_VAULT";
}

vector<GoalEntry> expandGoalEntriesForDisplay(const vector<GoalEntry>& entries) {
  vector<GoalEntry> expanded;

  auto trim = [](const string& value) -> string {
    size_t start = value.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = value.find_last_not_of(" \t\n\r");
    return value.substr(start, end - start + 1);
  };

  for (const auto& entry : entries) {
    if (entry.goalExpression.find('^') == string::npos) {
      expanded.push_back(entry);
      continue;
    }

    vector<string> parts;
    if (!entry.preconditions.empty()) {
      parts = entry.preconditions;
    } else {
      string current;
      for (char ch : entry.goalExpression) {
        if (ch == '^') {
          if (!trim(current).empty()) parts.push_back(trim(current));
          current.clear();
        } else {
          current.push_back(ch);
        }
      }
      if (!trim(current).empty()) parts.push_back(trim(current));
    }

    if (parts.empty()) {
      expanded.push_back(entry);
      continue;
    }

    for (const auto& part : parts) {
      GoalEntry clone = entry;
      clone.goalExpression = trim(part);
      clone.isOperator = false;
      clone.operatorName.clear();
      clone.preconditions.clear();
      clone.effects.clear();
      expanded.push_back(clone);
    }
  }

  return expanded;
}

} // namespace

GameEngineGUI::GameEngineGUI()
    : controlledPoliceIndex(0), difficulty(1), turnCount(0),
      gameStatus(GameStatus::RUNNING), vaultCollected(false),
      vaultPopupFrames(0), policeBoostTurnsRemaining(0),
  policeDoubleStepActive(false), debugWindowPid(-1),
  debugSnapshotPath("debug_snapshot.txt"), hoveredCell(0, 0, 0),
  hasHoveredCell(false) {}

GameEngineGUI::~GameEngineGUI() {
  stopDebugWindow();
}

void GameEngineGUI::initialize(int diff, PlayerRole role) {
  difficulty = diff;
  playerRole = role;
  turnCount = 0;
  gameStatus = GameStatus::RUNNING;
  statusMessage = "Game Started!";
  vaultCollected = false;
  vaultPopupFrames = 0;
  popupMessage.clear();
  policeBoostTurnsRemaining = 0;
  policeDoubleStepActive = false;
  policeList.clear();

  // Initialize grid
  int mapDifficulty = difficulty;
  if (playerRole == PlayerRole::POLICE) {
    // When human plays Police (Robber AI active), keep Easy as a single-floor map.
    if (difficulty == 1) mapDifficulty = 1;
    else mapDifficulty = 4 - difficulty; // 2->2, 3->1
  }
  grid.loadLevel(mapDifficulty);

  // Initialize AI engines
  heuristic = make_unique<HeuristicEngine>(grid);

  predictor = make_unique<PredictionEngine>(grid, 10 + difficulty * 2);

  rules = make_unique<RuleEngine>(difficulty);

  // Initialize robber(s)
  robbers.clear();
  robbers.push_back(make_unique<RobberAI>(grid.getInitialRobberPos(), 0, heuristic.get()));
  robbers[0]->setRuleEngine(rules.get());

  // Initialize police strictly from hardcoded 'P' positions in Grid3D maps.
  vector<Position> hardcodedPoliceSpawns = grid.getInitialPolicePos();
  if (hardcodedPoliceSpawns.empty()) {
    // Deterministic emergency fallback if a map is missing P markers.
    hardcodedPoliceSpawns.push_back(Position(1, 1, 0));
  }

  for (size_t i = 0; i < hardcodedPoliceSpawns.size(); ++i) {
    const Position &policeStart = hardcodedPoliceSpawns[i];
    bool hardMode = (difficulty >= 3);  // Hard mode for difficulty >= 3
    auto police = make_unique<PoliceAI>(policeStart, 
                                        static_cast<int>(i),
                                        predictor.get(),
                                        heuristic.get(), 
                                        rules.get(),
                                        hardMode);
    policeList.push_back(std::move(police));
  }

  {
    // Make spawn floors/coords visible at start so placement is explicit.
    std::ostringstream spawnInfo;
    spawnInfo << "Police spawns: ";
    for (size_t i = 0; i < policeList.size(); ++i) {
      const Position p = policeList[i]->getPosition();
      if (i > 0) {
        spawnInfo << " | ";
      }
      // 1-based row/col/floor for easier matching with map strings.
      spawnInfo << "P" << (i + 1) << "(r" << (p.y + 1) << ",c" << (p.x + 1)
                << ",f" << (p.z + 1) << ")";
    }
    statusMessage = spawnInfo.str();
  }

  controlledPoliceIndex = 0;
  if (playerRole == PlayerRole::POLICE) {
    syncControlledPoliceToRobberFloor();
  }

  // Initialize renderer
  renderer = make_unique<RaylibRenderer>(grid);
  renderer->initWindow();

  // Debug snapshot windows - spawn for AI-controlled agents
  debugSnapshotPaths.clear();
  debugWindowPids.clear();
  robberDebugSnapshotPaths.clear();
  robberDebugWindowPids.clear();
  
  if (playerRole == PlayerRole::ROBBER) {
    // User playing as Robber: show Police AI debug windows
    for (size_t i = 0; i < policeList.size(); ++i) {
      string path = "debug_snapshot_P" + to_string(i + 1) + ".txt";
      debugSnapshotPaths.push_back(path);
      spawnDebugWindowForPolice(i, path);
    }
  } else {
    // User playing as Police: show Robber AI debug windows
    for (size_t i = 0; i < robbers.size(); ++i) {
      string path = "debug_snapshot_R" + to_string(i + 1) + ".txt";
      robberDebugSnapshotPaths.push_back(path);
      spawnDebugWindowForRobber(i, path);
    }
  }

  hoveredCell = getPlayerPosition();
  hasHoveredCell = true;
}

void GameEngineGUI::run() {
  while (renderer->isWindowOpen()) {
    if (gameStatus == GameStatus::RUNNING) {
      handleInput();
      update();
      checkWinConditions();
    } else {
      if (IsKeyPressed(KEY_R)) {
        initialize(difficulty, playerRole);
        continue;
      }
      if (IsKeyPressed(KEY_ESCAPE)) {
        gameStatus = GameStatus::QUIT;
        break;
      }
    }

    render();
  }
}

void GameEngineGUI::handleInput() {
  Position currentPos = getPlayerPosition();
  Position targetPos = currentPos;

  bool requestedMove = false;

  if (playerRole == PlayerRole::ROBBER) {
    if (IsKeyPressed(KEY_W)) {
      targetPos.y = currentPos.y - 1;
      requestedMove = true;
    } else if (IsKeyPressed(KEY_S)) {
      targetPos.y = currentPos.y + 1;
      requestedMove = true;
    } else if (IsKeyPressed(KEY_A)) {
      targetPos.x = currentPos.x - 1;
      requestedMove = true;
    } else if (IsKeyPressed(KEY_D)) {
      targetPos.x = currentPos.x + 1;
      requestedMove = true;
    }
  } else {
    if (IsKeyPressed(KEY_W)) {
      targetPos.y = currentPos.y - 1;
      requestedMove = true;
    } else if (IsKeyPressed(KEY_S)) {
      targetPos.y = currentPos.y + 1;
      requestedMove = true;
    } else if (IsKeyPressed(KEY_A)) {
      targetPos.x = currentPos.x - 1;
      requestedMove = true;
    } else if (IsKeyPressed(KEY_D)) {
      targetPos.x = currentPos.x + 1;
      requestedMove = true;
    }
  }

  if (requestedMove) {
    tryMovePlayer(targetPos);
    return;
  }

  // Quit
  if (IsKeyPressed(KEY_ESCAPE)) {
    gameStatus = GameStatus::QUIT;
  }
}

void GameEngineGUI::update() {
  refreshVaultCollectionState();

  if (vaultPopupFrames > 0) {
    vaultPopupFrames--;
  }
}

void GameEngineGUI::render() {
    if (!renderer) return;
    
    // Collect police positions for rendering
    vector<Position> policePos;
    for (const auto& p : policeList) {
        policePos.push_back(p->getPosition());
    }
    
    // Render the game using Raylib
    bool showPopup = vaultPopupFrames > 0;
    bool showGameOver = (gameStatus != GameStatus::RUNNING);

    string gameOverTitle;
    if (gameStatus == GameStatus::POLICE_WON) {
        gameOverTitle = "Police Win!";
    } else if (gameStatus == GameStatus::ROBBER_WON) {
        gameOverTitle = "Robber Win!";
    } else {
        gameOverTitle = "No Wins!";
    }
    string gameOverPrompt = "Press R to restart";

    renderer->render(robbers.empty() ? Position(0,0,0) : robbers[0]->getPosition(),
                    policePos,
                    turnCount,
                    statusMessage,
                    vaultCollected,
                    showPopup,
                    popupMessage,
                    showGameOver,
                    gameOverTitle,
                    gameOverPrompt);
    
    // Write debug snapshots based on player role
    if (playerRole == PlayerRole::ROBBER) {
        // User is Robber: write Police AI snapshots
        for (size_t i = 0; i < policeList.size(); ++i) {
            if (i >= debugSnapshotPaths.size()) break;
            const string tmpPath = debugSnapshotPaths[i] + ".tmp";
            {
                ofstream f(tmpPath, ios::trunc);
                f << buildDebugSnapshotForPolice(i);
            }
            error_code ec;
            filesystem::remove(debugSnapshotPaths[i], ec);
            filesystem::rename(tmpPath, debugSnapshotPaths[i], ec);
        }
    } else {
        // User is Police: write Robber AI snapshots
        for (size_t i = 0; i < robbers.size(); ++i) {
            if (i >= robberDebugSnapshotPaths.size()) break;
            const string tmpPath = robberDebugSnapshotPaths[i] + ".tmp";
            {
                ofstream f(tmpPath, ios::trunc);
                f << buildDebugSnapshotForRobber(i);
            }
            error_code ec;
            filesystem::remove(robberDebugSnapshotPaths[i], ec);
            filesystem::rename(tmpPath, robberDebugSnapshotPaths[i], ec);
        }
    }
}

bool GameEngineGUI::tryMovePlayer(const Position &targetPos) {
  Position currentPos = getPlayerPosition();
  int dx = targetPos.x - currentPos.x;
  int dy = targetPos.y - currentPos.y;
  int dz = targetPos.z - currentPos.z;

  Position resolvedTarget = targetPos;

  if (playerRole == PlayerRole::POLICE) {
    if (!processRobberFirstTurnInPoliceMode()) {
      return true;
    }

    // Robber may change floors at turn start; apply input to newly controlled
    // same-floor police.
    Position updatedControlledPos = getPlayerPosition();
    resolvedTarget =
        Position(updatedControlledPos.x + dx, updatedControlledPos.y + dy,
                 updatedControlledPos.z + dz);
  }

  if (!isControllableMoveValid(resolvedTarget)) {
    statusMessage = "Blocked: boundary or wall";
  } else {
    setPlayerPosition(resolvedTarget);
    refreshVaultCollectionState();

    // In alert boost, player-controlled police can sprint 2 cells with Shift +
    // WASD.
    bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (playerRole == PlayerRole::POLICE && policeDoubleStepActive && shiftDown) {
      Position secondStep = getPlayerPosition();
      secondStep.x += dx;
      secondStep.y += dy;
      secondStep.z += dz;

      if (isControllableMoveValid(secondStep)) {
        setPlayerPosition(secondStep);
      } else {
        statusMessage = "Second step blocked";
      }
    }
  }

  if (!robbers.empty() && robbers[0]->hasReachedExit(grid) && vaultCollected) {
    gameStatus = GameStatus::ROBBER_WON;
    statusMessage = "Robber Escaped with Vault";
    return true;
  }

  if (!robbers.empty() && robbers[0]->hasReachedExit(grid) && !vaultCollected) {
    gameStatus = GameStatus::ROBBER_ESCAPED_NO_VAULT;
    statusMessage = "Robber Escaped without Vault";
    return true;
  }

  hoveredCell = getPlayerPosition();
  hasHoveredCell = true;
  updateAI();
  turnCount++;
  return true;
}

bool GameEngineGUI::processRobberFirstTurnInPoliceMode() {
  vector<Position> policePositions;
  for (const auto &p : policeList) {
    policePositions.push_back(p->getPosition());
  }

  Position robberMove = robbers[0]->getNextMove(grid, policePositions);
  if (robbers[0]->canMoveTo(robberMove, grid)) {
    robbers[0]->setPosition(robberMove);
    robbers[0] ? applyRobberFloorTransition(*robbers[0], grid) : (void)0;
    refreshVaultCollectionState();

    // If AI robber steps on alert, police boost should unlock immediately.
    if (grid.isAlertZone(robbers[0]->getPosition())) {
      policeBoostTurnsRemaining = 5;
      policeDoubleStepActive = true;
      grid.setCell(robbers[0]->getPosition(), CellType::CCTV_ZONE);
      if (rules) {
        rules->notifyAlertTriggered(robbers[0]->getPosition());
      }
      popupMessage = "Alert Triggered!";
      vaultPopupFrames = 120;
      statusMessage = "🚨 ALERT ZONE! Police move x2";
    }
  }

  syncControlledPoliceToRobberFloor();

  if (robbers[0]->hasReachedExit(grid) && vaultCollected) {
    gameStatus = GameStatus::ROBBER_WON;
    statusMessage = "Robber Escaped with Vault";
    return false;
  }

  if (robbers[0]->hasReachedExit(grid) && !vaultCollected) {
    gameStatus = GameStatus::ROBBER_ESCAPED_NO_VAULT;
    statusMessage = "Robber Escaped without Vault";
    return false;
  }

  for (const auto &police : policeList) {
    if (police->getPosition() == robbers[0]->getPosition()) {
      gameStatus = GameStatus::POLICE_WON;
      statusMessage = "Police Caught Robber";
      return false;
    }
  }

  return true;
}

void GameEngineGUI::syncControlledPoliceToRobberFloor() {
  if (playerRole != PlayerRole::POLICE || policeList.empty()) {
    return;
  }

  int robberFloor = robbers.empty() ? 0 : robbers[0]->getPosition().z;

  if (controlledPoliceIndex >= 0 &&
      controlledPoliceIndex < static_cast<int>(policeList.size()) &&
      policeList[controlledPoliceIndex]->getPosition().z == robberFloor) {
    return;
  }

  int bestIndex = -1;
  int bestDistance = std::numeric_limits<int>::max();
  Position robberPos = robbers.empty() ? Position(0,0,0) : robbers[0]->getPosition();

  for (size_t i = 0; i < policeList.size(); ++i) {
    Position p = policeList[i]->getPosition();
    if (p.z != robberFloor) {
      continue;
    }

    int dist = abs(p.x - robberPos.x) + abs(p.y - robberPos.y);
    if (dist < bestDistance) {
      bestDistance = dist;
      bestIndex = static_cast<int>(i);
    }
  }

  if (bestIndex != -1) {
    controlledPoliceIndex = bestIndex;
  }
}

bool GameEngineGUI::isControllableMoveValid(const Position &targetPos) const {
  Position currentPos = getPlayerPosition();
  if (!grid.isValid(targetPos)) {
    return false;
  }

  int delta = abs(targetPos.x - currentPos.x) +
              abs(targetPos.y - currentPos.y) + abs(targetPos.z - currentPos.z);
  if (delta != 1) {
    return false;
  }

  if (playerRole == PlayerRole::ROBBER) {
    return !robbers.empty() && robbers[0]->canMoveTo(targetPos, grid);
  }

  if (policeList.empty() || controlledPoliceIndex < 0 ||
      controlledPoliceIndex >= static_cast<int>(policeList.size())) {
    return false;
  }

  for (size_t i = 0; i < policeList.size(); ++i) {
    if (static_cast<int>(i) != controlledPoliceIndex &&
        policeList[i]->getPosition() == targetPos) {
      return false;
    }
  }

  return policeList[controlledPoliceIndex]->canMoveTo(targetPos, grid);
}

Position GameEngineGUI::getMouseGridPosition() const {
  Vector2 mouse = GetMousePosition();
  int cellSize = renderer->getCellSize();
  int startX = renderer->getGridStartX();
  int startY = renderer->getGridStartY();

  int x = static_cast<int>((mouse.x - startX) / cellSize);
  int y = static_cast<int>((mouse.y - startY) / cellSize);

  x = std::max(0, std::min(x, grid.getWidth() - 1));
  y = std::max(0, std::min(y, grid.getHeight() - 1));

  return Position(x, y, getPlayerPosition().z);
}

Position GameEngineGUI::getPlayerPosition() const {
  if (playerRole == PlayerRole::ROBBER) {
    return robbers.empty() ? Position(0,0,0) : robbers[0]->getPosition();
  }

  if (policeList.empty() || controlledPoliceIndex < 0 ||
      controlledPoliceIndex >= static_cast<int>(policeList.size())) {
    return Position(0, 0, 0);
  }

  return policeList[controlledPoliceIndex]->getPosition();
}

void GameEngineGUI::setPlayerPosition(const Position &newPos) {
  if (playerRole == PlayerRole::ROBBER) {
    robbers[0]->setPosition(newPos);

    Position updatedPos = robbers[0]->getPosition();
    CellType atCell = grid.getCell(updatedPos);
    if (atCell == CellType::STAIRS) {
      Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
      if (grid.isValid(up) && !grid.isWall(up)) {
        if (!robbers.empty() && !isPoliceOnCell(up)) robbers[0]->setPosition(up);
        else statusMessage = "Blocked: police at upstairs landing";
      }
    } else if (atCell == CellType::ELEVATOR) {
      Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
      if (grid.isValid(down) && !grid.isWall(down)) {
        if (!robbers.empty() && !isPoliceOnCell(down)) robbers[0]->setPosition(down);
        else statusMessage = "Blocked: police at downstairs landing";
      }
    }

    return;
  }

  if (!policeList.empty() && controlledPoliceIndex >= 0 &&
      controlledPoliceIndex < static_cast<int>(policeList.size())) {
    policeList[controlledPoliceIndex]->setPosition(newPos);

    if (shouldPoliceTransitionFloors(
            policeList[controlledPoliceIndex]->getPosition())) {
      Position updatedPos = policeList[controlledPoliceIndex]->getPosition();
      CellType atCell = grid.getCell(updatedPos);
      if (atCell == CellType::STAIRS) {
        Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
        if (grid.isValid(up) && !grid.isWall(up)) {
          policeList[controlledPoliceIndex]->setPosition(up);
        } else {
          Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
          if (grid.isValid(down) && !grid.isWall(down)) {
            policeList[controlledPoliceIndex]->setPosition(down);
          }
        }
      }
    }
  }
}

bool GameEngineGUI::isPoliceOnCell(const Position &pos) const {
  for (const auto &police : policeList) {
    if (police->getPosition() == pos) {
      return true;
    }
  }
  return false;
}

bool GameEngineGUI::arePoliceFloorChangesAllowed() const {
  return vaultCollected || policeDoubleStepActive;
}

bool GameEngineGUI::shouldPoliceTransitionFloors(
    const Position &policePos) const {
  // Police can only transition if vault is collected AND robber is on the
  // Police can only transition if vault is collected OR police boost is active
  if (!arePoliceFloorChangesAllowed()) {
    return false;
  }

  CellType cellType = grid.getCell(policePos);
  if (cellType == CellType::STAIRS) {
    return true;
  }

  return false;
}

void GameEngineGUI::updateAI() {
  policeDoubleStepActive = policeBoostTurnsRemaining > 0;

  auto publishDebugSnapshot = [&]() {
    const string tempSnapshotPath = debugSnapshotPath + ".tmp";
    {
      ofstream snapshot(tempSnapshotPath, ios::trunc);
      snapshot << buildDebugSnapshot();
    }
    std::error_code renameError;
    std::filesystem::remove(debugSnapshotPath, renameError);
    std::filesystem::rename(tempSnapshotPath, debugSnapshotPath, renameError);
  };

  auto pauseForDebug = [&](int seconds) {
    publishDebugSnapshot();
    // removed artificial delay: publish snapshot immediately without sleeping
  };

  vector<Position> policePositions;
  for (const auto &p : policeList) {
    policePositions.push_back(p->getPosition());
  }

  const Position currentRobberPos = robbers[0]->getPosition();
  const bool robberMoved = !(currentRobberPos == lastRobberPos);

  if (playerRole == PlayerRole::ROBBER) {
    if (robberMoved && grid.isAlertZone(currentRobberPos)) {
      policeBoostTurnsRemaining = 5;
      grid.setCell(currentRobberPos, CellType::CCTV_ZONE);
      if (rules) {
        rules->notifyAlertTriggered(currentRobberPos);
      }

      // Do not push manual generic respond goals here.
      // Each police planner handles alert interrupts and pushes the
      // indexed goal (e.g., respond(Police1, Alert)) exactly once.
      popupMessage = "Alert Triggered!";
      vaultPopupFrames = 120;
    }

    set<Position> occupiedPolicePositions;
    for (const auto &police : policeList) {
      occupiedPolicePositions.insert(police->getPosition());
    }

    // Detect robber movement (first-move-on-floor events)
    // Only trigger when the robber changed floors (first move onto a different floor)
    if (!vaultCollected && robberMoved && currentRobberPos.z != lastRobberPos.z) {
      // For each police on the same floor where the robber moved, or on an adjacent floor,
      // push a protective goal to locally protect (e.g., move to Upstairs/Downstairs)
      for (size_t i = 0; i < policeList.size(); ++i) {
        auto &police = policeList[i];
        int pz = police->getPosition().z;
        int rz = currentRobberPos.z;
        int dz = abs(pz - rz);
        if (dz == 0 || dz == 1) {
          if (grid.getVaultPos().z != rz) {
            // push protect goal targeting a local floor transition (Upstairs/Downstairs)
            string dir = (rz > pz) ? "Upstairs" : "Downstairs";
            string expr = string("protect(Police,") + dir + ")";
            if (police->getPlanner()) {
              police->getPlanner()->pushExternalGoal(expr,
                                                    {string("sameFloor(Police, ") + to_string(rz) + ")", string("pathAvailable(Police, ")+to_string(rz)+")"},
                                                    {});
            }
          }
        }
      }
    }
    lastRobberPos = currentRobberPos;

    for (size_t i = 0; i < policeList.size(); ++i) {
      auto &police = policeList[i];
      occupiedPolicePositions.erase(police->getPosition());

      police->setVaultCollected(vaultCollected);

      statusMessage = string("Police planning: calculate goal for P") + to_string(i + 1);
      publishDebugSnapshot();

      statusMessage = string("Police planning: execute move for P") + to_string(i + 1);
      Position policeMove = police->getNextMove(grid, vector<Position>{robbers[0]->getPosition()});
      publishDebugSnapshot();

      if (policeMove == police->getPosition()) {
        occupiedPolicePositions.insert(police->getPosition());
        continue;
      }

      Position previousPolicePos = police->getPosition();
      if (police->canMoveTo(policeMove, grid) &&
          !occupiedPolicePositions.count(policeMove)) {
        police->setPosition(policeMove);

        // Avoid double floor hops: if planner already changed z this turn,
        // do not apply GUI fallback transition again.
        if (police->getPosition().z == previousPolicePos.z &&
            shouldPoliceTransitionFloors(police->getPosition())) {
          Position updatedPos = police->getPosition();
          CellType atCell = grid.getCell(updatedPos);
          if (atCell == CellType::STAIRS) {
            if (policeMove.z < previousPolicePos.z) {
              Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
              if (grid.isValid(down) && !grid.isWall(down)) {
                police->setPosition(down);
              }
            } else {
              Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
              if (grid.isValid(up) && !grid.isWall(up)) {
                police->setPosition(up);
              }
            }
          }
        }
      }
      occupiedPolicePositions.insert(police->getPosition());
    }
  } else {
    set<Position> occupiedPolicePositions;
    for (const auto &police : policeList) {
      occupiedPolicePositions.insert(police->getPosition());
    }

    for (size_t i = 0; i < policeList.size(); ++i) {
      if (static_cast<int>(i) == controlledPoliceIndex) {
        continue;
      }
      vector<Position> robberAsTarget{robbers[0]->getPosition()};

      // Tell police about vault status so they know if they can use stairs
      policeList[i]->setVaultCollected(vaultCollected);
      statusMessage = string("Police planning: calculate goal for P") + to_string(i + 1);
      publishDebugSnapshot();

      Position policeMove = policeList[i]->getNextMove(grid, robberAsTarget);

      statusMessage = string("Police planning: execute move for P") + to_string(i + 1);

      if (occupiedPolicePositions.count(policeMove) &&
          policeMove != robbers[0]->getPosition()) {
        continue;
      }

      Position previousPolicePos = policeList[i]->getPosition();
      if (policeList[i]->canMoveTo(policeMove, grid)) {
        policeList[i]->setPosition(policeMove);

        // Avoid double floor hops: if planner already changed z this turn,
        // do not apply GUI fallback transition again.
        if (policeList[i]->getPosition().z == previousPolicePos.z &&
            shouldPoliceTransitionFloors(policeList[i]->getPosition())) {
          Position updatedPos = policeList[i]->getPosition();
          CellType atCell = grid.getCell(updatedPos);
          if (atCell == CellType::STAIRS) {
            if (policeMove.z < previousPolicePos.z) {
              Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
              if (grid.isValid(down) && !grid.isWall(down)) {
                policeList[i]->setPosition(down);
              }
            } else {
              Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
              if (grid.isValid(up) && !grid.isWall(up)) {
                policeList[i]->setPosition(up);
              }
            }
          }
        }

        occupiedPolicePositions.insert(policeList[i]->getPosition());
      }
      publishDebugSnapshot();
    }
  }

  if (policeBoostTurnsRemaining > 0) {
    policeBoostTurnsRemaining--;
  }
  policeDoubleStepActive = policeBoostTurnsRemaining > 0;

  // Update rules
  GameContext ctx;
  ctx.playerDetected = false;
  ctx.cctvTriggered = false;
  ctx.alertTriggered = policeDoubleStepActive || (rules && rules->isAlertActive());
  ctx.turnsInAlert = policeBoostTurnsRemaining;
  ctx.policeSpeed = policeDoubleStepActive ? 2.0f : 1.0f;
  ctx.predictionDepth = 10 + difficulty * 2;
  ctx.turnsAtLarge = turnCount;
  ctx.vaultStolen = vaultCollected;
  ctx.alertPos = rules ? rules->getAlertPos() : Position();

  rules->updateContext(ctx);

  // Update status message
  if (ctx.alertTriggered) {
    statusMessage = "🚨 ALERT ZONE! Police move x2";
  } else if (vaultCollected) {
    statusMessage = "💰 Vault secured - Reach EXIT";
  } else {
    statusMessage = "🕵️ All Clear - Reach VAULT";
  }
}

void GameEngineGUI::checkWinConditions() {
  if (robbers[0]->hasReachedExit(grid) && vaultCollected) {
    gameStatus = GameStatus::ROBBER_WON;
    statusMessage = "Robber Escaped with Vault";
    return;
  }

  if (robbers[0]->hasReachedExit(grid) && !vaultCollected) {
    gameStatus = GameStatus::ROBBER_ESCAPED_NO_VAULT;
    statusMessage = "Robber Escaped without Vault";
    return;
  }

  // Police wins if catches robber
  for (const auto &police : policeList) {
    if (police->getPosition() == robbers[0]->getPosition()) {
      gameStatus = GameStatus::POLICE_WON;
      statusMessage = "Police Caught Robber";
      return;
    }
  }
}

void GameEngineGUI::refreshVaultCollectionState() {
  if (!vaultCollected && robbers[0]->getPosition() == grid.getVaultPos()) {
    vaultCollected = true;
    // Clear vault tile so GUI reflects collection immediately
    grid.setCell(grid.getVaultPos(), CellType::EMPTY);
    if (rules) {
      rules->notifyVaultStolen();
    }
    popupMessage = "Vault Collected!";
    vaultPopupFrames = 150;
    statusMessage = "💰 Vault secured - Reach EXIT";
    // Notify all police planners immediately so they cancel and push catch blocks.
    for (size_t i = 0; i < policeList.size(); ++i) {
      auto &police = policeList[i];
      if (police->getPlanner()) {
        WorldState ws;
        ws.grid = &grid;
        ws.policePos = police->getPosition();
        ws.robberPos = robbers[0]->getPosition();
        ws.vaultPos = grid.getVaultPos();
        ws.exitPos = grid.getExitPos();
        ws.alertPos = rules ? rules->getAlertPos() : Position();
        ws.robberHasVault = vaultCollected;
        ws.vaultStolen = true;
        ws.alertTriggered = rules ? rules->isAlertActive() : false;
        ws.boostActive = rules ? rules->isBoostActive() : false;
        // collect all police positions
        vector<Position> ppos;
        for (const auto &p : policeList) ppos.push_back(p->getPosition());
        ws.policePositions = ppos;
        police->getPlanner()->onVaultStolen(ws);
      }
    }
  }
}

static string toUpperCopy(string value) {
  for (char &ch : value) {
    ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
  }
  return value;
}

static string formatCellLabel(const Position& pos) {
  stringstream ss;
  ss << "Cell (" << pos.y << "," << pos.x << ")";
  return ss.str();
}

string GameEngineGUI::buildDebugSnapshot() const {
  auto goalTypeToString = [](GoalType type) {
    switch (type) {
      case GoalType::PROTECT_VAULT: return string("PROTECT_VAULT");
      case GoalType::CHASE_ROBBER: return string("CHASE_ROBBER");
      case GoalType::RESPOND_ALERT: return string("RESPOND_ALERT");
      case GoalType::REACH_VAULT: return string("REACH_VAULT");
      case GoalType::ESCAPE_TO_EXIT: return string("ESCAPE_TO_EXIT");
      case GoalType::REACH_ALERT_ZONE: return string("REACH_ALERT_ZONE");
      case GoalType::MOVE_FLOOR_GOAL: return string("MOVE_FLOOR_GOAL");
      default: return string("NONE");
    }
  };

  auto statusToString = [](GoalEntry::Status status) {
    switch (status) {
      case GoalEntry::ACTIVE: return string("ACTIVE");
      case GoalEntry::PERFORMING: return string("PERFORMING");
      case GoalEntry::COMPLETED: return string("COMPLETED");
      case GoalEntry::CANCELLED: return string("CANCELLED");
      default: return string("PENDING");
    }
  };

  auto jsonString = [](const string& value) {
    string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
      if (ch == '"' || ch == '\\') {
        escaped.push_back('\\');
      }
      escaped.push_back(ch);
    }
    return string("\"") + escaped + "\"";
  };

  auto jsonArray = [&](const vector<string>& values) {
    stringstream arr;
    arr << "[";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) arr << ", ";
      arr << jsonString(values[i]);
    }
    arr << "]";
    return arr.str();
  };

  auto jsonPos = [](const Position& pos) {
    stringstream ss;
    ss << "[" << pos.x << ", " << pos.y << ", " << pos.z << "]";
    return ss.str();
  };

  auto jsonGoalEntry = [&](const GoalEntry& entry) {
    stringstream ss;
    ss << "{\n";
    ss << "      \"expression\": " << jsonString(entry.goalExpression) << ",\n";
    ss << "      \"isOperator\": " << (entry.isOperator ? "true" : "false") << ",\n";
    ss << "      \"operatorName\": " << jsonString(entry.operatorName) << ",\n";
    ss << "      \"status\": " << jsonString(statusToString(entry.status)) << ",\n";
    ss << "      \"preconditions\": " << jsonArray(entry.preconditions) << ",\n";
    ss << "      \"effects\": " << jsonArray(entry.effects) << "\n";
    ss << "    }";
    return ss.str();
  };

  auto jsonNode = [&](const Node& node) {
    stringstream ss;
    ss << "{\n";
    ss << "      \"label\": " << jsonString(positionToString(node.pos)) << ",\n";
    ss << "      \"pos\": " << jsonPos(node.pos) << ",\n";
    ss << "      \"g\": " << node.g << ",\n";
    ss << "      \"h\": " << node.h << ",\n";
    ss << "      \"f\": " << node.f << ",\n";
    ss << "      \"chosen\": " << ((node.pos == AStar3D::getLastSearchTrace().chosenNode) ? "true" : "false") << "\n";
    ss << "    }";
    return ss.str();
  };

  // Lambda to extract target name from position
  auto getTargetName = [&](const Position& pos) -> string {
    if (pos == grid.getVaultPos()) return "Vault";
    if (pos == grid.getExitPos()) return "Exit";
    if (pos == robbers[0]->getPosition()) return "Robber";
    stringstream ss;
    ss << "[" << pos.x << ", " << pos.y << ", " << pos.z << "]";
    return ss.str();
  };

  // Lambda to check if two positions are adjacent (4-directional, no diagonals)
  auto areAdjacentCells = [](const Position& a, const Position& b) {
    if (a.z != b.z) return false; // Must be same floor
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
  };

  string agent = "Robber";
  string currentState;
  Position currentPos;
  Position targetPos;
  vector<GoalEntry> activeGoals;
  vector<GoalEntry> completedGoals;
  vector<GoalEntry> cancelledGoals;
  vector<Node> successors;
  SearchTrace trace;

  // Prefer planner-local A* trace if available
  if (playerRole == PlayerRole::ROBBER) {
    if (!policeList.empty()) {
      int policeIndex = std::min(controlledPoliceIndex, static_cast<int>(policeList.size() - 1));
      if (policeList[policeIndex]->getPlanner() && policeList[policeIndex]->getPlanner()->hasAstarTrace()) {
        trace = policeList[policeIndex]->getPlanner()->getLastAstarTrace();
        successors = trace.immediateSuccessors;
      }
    }
  } else {
    if (robbers[0]->getPlanner() && robbers[0]->getPlanner()->hasAstarTrace()) {
      trace = robbers[0]->getPlanner()->getLastAstarTrace();
      successors = trace.immediateSuccessors;
    }
  }

  // fallback to global A* trace
  if (successors.empty()) {
    successors = AStar3D::getSuccessorLog();
    trace = AStar3D::getLastSearchTrace();
  }

  const bool hasTrace = trace.goalType != GoalType::NONE || !successors.empty();

  if (playerRole == PlayerRole::ROBBER) {
    agent = "Police";
    if (!policeList.empty()) {
      int policeIndex = std::min(controlledPoliceIndex, static_cast<int>(policeList.size() - 1));
      currentPos = policeList[policeIndex]->getPosition();
      currentState = policeStateName(policeList[policeIndex]->getState());
      
      // Extract target dynamically from police's goal stack
      targetPos = grid.getVaultPos(); // Default to vault
      if (policeList[policeIndex]->getPlanner()) {
        activeGoals = policeList[policeIndex]->getPlanner()->getGoalStack().getStack();
        completedGoals = policeList[policeIndex]->getPlanner()->getGoalStack().getCompletedGoals();
        cancelledGoals = policeList[policeIndex]->getPlanner()->getGoalStack().getCancelledGoals();
        
        // Look at the active goal to determine target
        const auto& stack = activeGoals;
        for (const auto& entry : stack) {
          if (entry.goalExpression.find("chase(Police, Robber)") != string::npos ||
              entry.goalExpression.find("at(Police, Robber)") != string::npos ||
              entry.goalExpression.find("sameFloor") != string::npos) {
            targetPos = robbers[0]->getPosition();
            break;
          } else if (entry.goalExpression.find("protect(Police, Vault)") != string::npos ||
                     entry.goalExpression.find("at(Police, Vault)") != string::npos) {
            targetPos = grid.getVaultPos();
            break;
          } else if (entry.goalExpression.find("respondAlert") != string::npos ||
                     entry.goalExpression.find("at(Police, Alert)") != string::npos) {
            targetPos = rules ? rules->getAlertPos() : Position();
            break;
          }
        }
      }
    }
  } else {
    agent = "Robber";
    currentPos = robbers[0]->getPosition();
    targetPos = vaultCollected ? grid.getExitPos() : grid.getVaultPos();
    currentState = robberStateName(robbers[0]->getState());
    if (robbers[0]->getPlanner()) {
      activeGoals = robbers[0]->getPlanner()->getGoalStack().getStack();
      completedGoals = robbers[0]->getPlanner()->getGoalStack().getCompletedGoals();
      cancelledGoals = robbers[0]->getPlanner()->getGoalStack().getCancelledGoals();
    }
  }

  // No synthetic active operator insertion — use planner-provided stack and operator statuses
  // (Planner sets PERFORMING status when it starts executing an operator)

  // Filter successors to only show moves adjacent to the A* search start (4-directional)
  // Use the trace.start because planners capture the A* trace before applying the first step
  Position filterCenter = trace.start;
  vector<Node> filteredSuccessors;
  for (const auto& node : successors) {
    if (areAdjacentCells(filterCenter, node.pos)) {
      filteredSuccessors.push_back(node);
    }
  }

  stringstream ss;
  ss << "{\n";
  ss << "  \"agent\": " << jsonString(agent) << ",\n";
  ss << "  \"state\": " << jsonString(currentState) << ",\n";
  int stepsTaken = 0;
  if (playerRole == PlayerRole::ROBBER) {
    if (!policeList.empty()) {
      int policeIndex = std::min(controlledPoliceIndex, static_cast<int>(policeList.size() - 1));
      stepsTaken = policeList[policeIndex]->getStepsTaken();
    }
  } else if (!robbers.empty()) {
    stepsTaken = robbers[0]->getStepsTaken();
  }
  ss << "  \"stepsTaken\": " << stepsTaken << ",\n";
  ss << "  \"currentCell\": " << jsonPos(currentPos) << ",\n";
  
  // Output targetCell: "None" if empty goal stack, otherwise format as "[x,y,z]" or label
  string targetCellStr = "None";
  if (!activeGoals.empty()) {
    targetCellStr = jsonPos(targetPos);
  }
  ss << "  \"targetCell\": " << targetCellStr << ",\n";
  
  const vector<GoalEntry> displayActiveGoals = expandGoalEntriesForDisplay(activeGoals);
  const vector<GoalEntry> displayCompletedGoals = expandGoalEntriesForDisplay(completedGoals);
  const vector<GoalEntry> displayCancelledGoals = expandGoalEntriesForDisplay(cancelledGoals);

  ss << "  \"goalStack\": [\n";
  for (size_t i = 0; i < displayActiveGoals.size(); ++i) {
    ss << "    " << jsonGoalEntry(displayActiveGoals[i]);
    if (i + 1 < displayActiveGoals.size()) ss << ",";
    ss << "\n";
  }
  ss << "  ],\n";
  ss << "  \"completedGoals\": [\n";
  for (size_t i = 0; i < displayCompletedGoals.size(); ++i) {
    ss << "    " << jsonGoalEntry(displayCompletedGoals[i]);
    if (i + 1 < displayCompletedGoals.size()) ss << ",";
    ss << "\n";
  }
  ss << "  ],\n";
  ss << "  \"cancelledGoals\": [\n";
  for (size_t i = 0; i < displayCancelledGoals.size(); ++i) {
    ss << "    " << jsonGoalEntry(displayCancelledGoals[i]);
    if (i + 1 < displayCancelledGoals.size()) ss << ",";
    ss << "\n";
  }
  ss << "  ],\n";
  ss << "  \"astarSuccessors\": [\n";
  for (size_t i = 0; i < filteredSuccessors.size(); ++i) {
    ss << "    " << jsonNode(filteredSuccessors[i]);
    if (i + 1 < filteredSuccessors.size()) ss << ",";
    ss << "\n";
  }
  ss << "  ],\n";
  ss << "  \"chosenNode\": {\n";
  const Node* chosenNode = nullptr;
  for (const auto& node : filteredSuccessors) {
    if (node.pos == trace.chosenNode) {
      chosenNode = &node;
      break;
    }
  }
  ss << "    \"label\": " << jsonString(positionToString(trace.chosenNode)) << ",\n";
  ss << "    \"pos\": " << jsonPos(trace.chosenNode) << ",\n";
  ss << "    \"g\": " << (chosenNode ? chosenNode->g : 0) << ",\n";
  ss << "    \"h\": " << (chosenNode ? chosenNode->h : 0) << ",\n";
  ss << "    \"f\": " << (chosenNode ? chosenNode->f : 0) << "\n";
  ss << "  },\n";
  ss << "  \"activeGoalType\": " << jsonString(goalTypeToString(trace.goalType)) << "\n";
  ss << "}\n";

  return ss.str();
}

void GameEngineGUI::spawnDebugWindowForPolice(int index, const string& path) {
#ifdef __APPLE__
    pid_t pid = fork();
    if (pid == 0) {
        // Pass police label as window title arg
        string title = "Police " + to_string(index + 1) + " Debug";
        execl("./debug_window", "./debug_window",
              path.c_str(), title.c_str(), (char*)nullptr);
        _exit(1);
    }
    if (pid > 0) debugWindowPids.push_back(pid);
#endif
}

void GameEngineGUI::stopDebugWindow() {
#ifdef __APPLE__
    for (pid_t pid : debugWindowPids) {
        if (pid > 0) kill(pid, SIGTERM);
    }
    for (pid_t pid : robberDebugWindowPids) {
        if (pid > 0) kill(pid, SIGTERM);
    }
    debugWindowPids.clear();
    robberDebugWindowPids.clear();
#endif
}

void GameEngineGUI::spawnDebugWindowForRobber(int index, const string& path) {
#ifdef __APPLE__
    pid_t pid = fork();
    if (pid == 0) {
        string title = "Robber " + to_string(index + 1) + " Debug";
        execl("./debug_window", "./debug_window",
              path.c_str(), title.c_str(), (char*)nullptr);
        _exit(1);
    }
    if (pid > 0) robberDebugWindowPids.push_back(pid);
#endif
}

string GameEngineGUI::buildDebugSnapshotForPolice(int idx) const {
    auto goalTypeToString = [](GoalType type) -> string {
        switch (type) {
            case GoalType::PROTECT_VAULT:   return "PROTECT_VAULT";
            case GoalType::CHASE_ROBBER:    return "CHASE_ROBBER";
            case GoalType::RESPOND_ALERT:   return "RESPOND_ALERT";
            case GoalType::REACH_VAULT:     return "REACH_VAULT";
            case GoalType::ESCAPE_TO_EXIT:  return "ESCAPE_TO_EXIT";
            case GoalType::REACH_ALERT_ZONE:return "REACH_ALERT_ZONE";
            case GoalType::MOVE_FLOOR_GOAL: return "MOVE_FLOOR_GOAL";
            default:                        return "NONE";
        }
    };

    auto statusToString = [](GoalEntry::Status s) -> string {
        switch (s) {
            case GoalEntry::ACTIVE:     return "ACTIVE";
            case GoalEntry::PERFORMING: return "PERFORMING";
            case GoalEntry::COMPLETED:  return "COMPLETED";
            case GoalEntry::CANCELLED:  return "CANCELLED";
            default:                    return "PENDING";
        }
    };

    auto jsonString = [](const string& v) -> string {
        string out = "\"";
        for (char c : v) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        out += '"';
        return out;
    };

    auto jsonArray = [&](const vector<string>& vals) -> string {
        string out = "[";
        for (size_t i = 0; i < vals.size(); ++i) {
            if (i) out += ", ";
            out += jsonString(vals[i]);
        }
        out += "]";
        return out;
    };

    auto jsonPos = [](const Position& p) -> string {
        return "[" + to_string(p.x) + ", " + to_string(p.y) + ", " + to_string(p.z) + "]";
    };

    auto jsonGoalEntry = [&](const GoalEntry& e) -> string {
        return "{\n"
               "      \"expression\": "   + jsonString(e.goalExpression)        + ",\n"
               "      \"isOperator\": "   + (e.isOperator ? "true" : "false")   + ",\n"
               "      \"operatorName\": " + jsonString(e.operatorName)           + ",\n"
               "      \"status\": "       + jsonString(statusToString(e.status)) + ",\n"
               "      \"preconditions\": "+ jsonArray(e.preconditions)           + ",\n"
               "      \"effects\": "      + jsonArray(e.effects)                 + "\n"
               "    }";
    };

    const auto& police       = policeList[idx];
    Position    currentPos   = police->getPosition();
    string      agentLabel   = "Police" + to_string(idx + 1);
    string      currentState = "PATROL";
    string      activeGoalTypeStr = "NONE";
    auto jsonNode = [&](const Node& n, bool chosen) -> string {
        return "{\n"
               "      \"label\": " + jsonString(positionToString(n.pos)) + ",\n"
               "      \"pos\": "   + jsonPos(n.pos)   + ",\n"
               "      \"g\": "     + to_string(n.g)   + ",\n"
               "      \"h\": "     + to_string(n.h)   + ",\n"
               "      \"f\": "     + to_string(n.g + n.h)   + ",\n"
               "      \"chosen\": "+ string(chosen ? "true" : "false") + "\n"
               "    }";
    };

    vector<GoalEntry> activeGoals, completedGoals, cancelledGoals;
    vector<Node>      successors;
    SearchTrace       trace;

    if (police->getPlanner()) {
        activeGoals    = police->getPlanner()->getGoalStack().getStack();
        completedGoals = police->getPlanner()->getGoalStack().getCompletedGoals();
        cancelledGoals = police->getPlanner()->getGoalStack().getCancelledGoals();

        if (police->getPlanner()->hasAstarTrace()) {
            trace      = police->getPlanner()->getLastAstarTrace();
            successors = trace.immediateSuccessors;
            activeGoalTypeStr = goalTypeToString(trace.goalType);
        }
    }

    // Determine target position from active goal expressions
    Position targetPos = grid.getVaultPos();
    for (const auto& e : activeGoals) {
        if (e.goalExpression.find("Robber") != string::npos) {
            targetPos = robbers[0]->getPosition(); break;
        }
        if (e.goalExpression.find("Alert") != string::npos) {
            targetPos = rules ? rules->getAlertPos() : Position(); break;
        }
        if (e.goalExpression.find("Vault") != string::npos) {
            targetPos = grid.getVaultPos(); break;
        }
    }

    // Filter successors to adjacents of trace start
    auto areAdj = [](const Position& a, const Position& b) {
        if (a.z != b.z) return false;
        return (abs(a.x-b.x) + abs(a.y-b.y)) == 1;
    };
    vector<Node> filtered;
    for (const auto& n : successors) {
        if (areAdj(trace.start, n.pos)) filtered.push_back(n);
    }

    // Build JSON
    stringstream ss;
    ss << "{\n";
    ss << "  \"agent\": "       << jsonString(agentLabel)   << ",\n";
    ss << "  \"state\": "       << jsonString(currentState) << ",\n";
    ss << "  \"currentCell\": " << jsonPos(currentPos)      << ",\n";
    ss << "  \"targetCell\": "  << jsonPos(targetPos)       << ",\n";

    const vector<GoalEntry> displayActiveGoals = expandGoalEntriesForDisplay(activeGoals);
    const vector<GoalEntry> displayCompletedGoals = expandGoalEntriesForDisplay(completedGoals);
    const vector<GoalEntry> displayCancelledGoals = expandGoalEntriesForDisplay(cancelledGoals);

    ss << "  \"goalStack\": [\n";
    for (size_t i = 0; i < displayActiveGoals.size(); ++i) {
      ss << "    " << jsonGoalEntry(displayActiveGoals[i]);
      if (i+1 < displayActiveGoals.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"completedGoals\": [\n";
    for (size_t i = 0; i < displayCompletedGoals.size(); ++i) {
      ss << "    " << jsonGoalEntry(displayCompletedGoals[i]);
      if (i+1 < displayCompletedGoals.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"cancelledGoals\": [\n";
    for (size_t i = 0; i < displayCancelledGoals.size(); ++i) {
      ss << "    " << jsonGoalEntry(displayCancelledGoals[i]);
      if (i+1 < displayCancelledGoals.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"astarSuccessors\": [\n";
    for (size_t i = 0; i < filtered.size(); ++i) {
        bool chosen = (filtered[i].pos == trace.chosenNode);
        ss << "    " << jsonNode(filtered[i], chosen);
        if (i+1 < filtered.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    // Chosen node
    const Node* cn = nullptr;
    for (const auto& n : filtered) {
        if (n.pos == trace.chosenNode) { cn = &n; break; }
    }
    if (!cn) {
        for (const auto& n : successors) {
            if (n.pos == trace.chosenNode) { cn = &n; break; }
        }
    }
    ss << "  \"chosenNode\": {\n";
    ss << "    \"label\": " << jsonString(positionToString(trace.chosenNode)) << ",\n";
    ss << "    \"pos\": "   << jsonPos(trace.chosenNode) << ",\n";
    ss << "    \"g\": "     << (cn ? cn->g : 0) << ",\n";
    ss << "    \"h\": "     << (cn ? cn->h : 0) << ",\n";
    ss << "    \"f\": "     << (cn ? cn->g + cn->h : 0) << "\n";
    ss << "  },\n";
    ss << "  \"activeGoalType\": " << jsonString(activeGoalTypeStr) << "\n";
    ss << "}\n";

    return ss.str();
}

string GameEngineGUI::buildDebugSnapshotForRobber(int idx) const {
    // Similar to buildDebugSnapshotForPolice but for robber(idx)
    if (idx >= static_cast<int>(robbers.size())) {
        return "{}";  // Return empty JSON if robber doesn't exist
    }

    auto goalTypeToString = [](GoalType type) -> string {
        switch (type) {
            case GoalType::REACH_VAULT:     return "REACH_VAULT";
            case GoalType::ESCAPE_TO_EXIT:  return "ESCAPE_TO_EXIT";
            default:                        return "NONE";
        }
    };

    auto statusToString = [](GoalEntry::Status s) -> string {
        switch (s) {
            case GoalEntry::ACTIVE:     return "ACTIVE";
            case GoalEntry::PERFORMING: return "PERFORMING";
            case GoalEntry::COMPLETED:  return "COMPLETED";
            case GoalEntry::CANCELLED:  return "CANCELLED";
            default:                    return "PENDING";
        }
    };

    auto jsonString = [](const string& v) -> string {
        string out = "\"";
        for (char c : v) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        out += '"';
        return out;
    };

    auto jsonArray = [&](const vector<string>& vals) -> string {
        string out = "[";
        for (size_t i = 0; i < vals.size(); ++i) {
            if (i) out += ", ";
            out += jsonString(vals[i]);
        }
        out += "]";
        return out;
    };

    auto jsonPos = [](const Position& p) -> string {
        return "[" + to_string(p.x) + ", " + to_string(p.y) + ", " + to_string(p.z) + "]";
    };

    auto jsonGoalEntry = [&](const GoalEntry& e) -> string {
        return "{\n"
               "      \"expression\": "   + jsonString(e.goalExpression)        + ",\n"
               "      \"isOperator\": "   + (e.isOperator ? "true" : "false")   + ",\n"
               "      \"operatorName\": " + jsonString(e.operatorName)           + ",\n"
               "      \"status\": "       + jsonString(statusToString(e.status)) + ",\n"
               "      \"preconditions\": "+ jsonArray(e.preconditions)           + ",\n"
               "      \"effects\": "      + jsonArray(e.effects)                 + "\n"
               "    }";
    };

    const auto& robber        = robbers[idx];
    Position    currentPos    = robbers[0]->getPosition();
    string      agentLabel    = "Robber" + to_string(idx + 1);
    string      currentState  = robberStateName(robbers[0]->getState());
    string      activeGoalTypeStr = "NONE";
    auto jsonNode = [&](const Node& n, bool chosen) -> string {
        return "{\n"
               "      \"label\": " + jsonString(positionToString(n.pos)) + ",\n"
               "      \"pos\": "   + jsonPos(n.pos)   + ",\n"
               "      \"g\": "     + to_string(n.g)   + ",\n"
               "      \"h\": "     + to_string(n.h)   + ",\n"
               "      \"f\": "     + to_string(n.g + n.h)   + ",\n"
               "      \"chosen\": "+ string(chosen ? "true" : "false") + "\n"
               "    }";
    };

    Position    targetPos     = vaultCollected ? grid.getExitPos() : grid.getVaultPos();
    vector<GoalEntry> activeGoals, completedGoals, cancelledGoals;
    vector<Node> successors;
    SearchTrace trace;

    // Get A* trace from robber's planner if available
    if (robbers[0]->getPlanner() && robbers[0]->getPlanner()->hasAstarTrace()) {
        trace = robbers[0]->getPlanner()->getLastAstarTrace();
        successors = trace.immediateSuccessors;
        activeGoalTypeStr = goalTypeToString(trace.goalType);
    }

    // Fallback to global trace
    if (successors.empty()) {
        successors = AStar3D::getSuccessorLog();
        trace = AStar3D::getLastSearchTrace();
    }

    // Get goal stack from planner
    if (robbers[0]->getPlanner()) {
        activeGoals = robbers[0]->getPlanner()->getGoalStack().getStack();
        completedGoals = robbers[0]->getPlanner()->getGoalStack().getCompletedGoals();
        cancelledGoals = robbers[0]->getPlanner()->getGoalStack().getCancelledGoals();
    }

    // Filter successors to adjacent cells only
    auto areAdjacentCells = [](const Position& a, const Position& b) {
        if (a.z != b.z) return false;
        int dx = abs(a.x - b.x);
        int dy = abs(a.y - b.y);
        return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
    };

    Position filterCenter = trace.start;
    vector<Node> filtered;
    for (const auto& node : successors) {
        if (areAdjacentCells(filterCenter, node.pos)) {
            filtered.push_back(node);
        }
    }

    stringstream ss;
    ss << "{\n";
    ss << "  \"agent\": " << jsonString(agentLabel) << ",\n";
    ss << "  \"state\": " << jsonString(currentState) << ",\n";
    ss << "  \"stepsTaken\": " << robber->getStepsTaken() << ",\n";
    ss << "  \"currentCell\": " << jsonPos(currentPos) << ",\n";

    string targetCellStr = "None";
    if (!activeGoals.empty()) {
        targetCellStr = jsonPos(targetPos);
    }
    ss << "  \"targetCell\": " << targetCellStr << ",\n";

    const vector<GoalEntry> displayActiveGoals = expandGoalEntriesForDisplay(activeGoals);
    const vector<GoalEntry> displayCompletedGoals = expandGoalEntriesForDisplay(completedGoals);
    const vector<GoalEntry> displayCancelledGoals = expandGoalEntriesForDisplay(cancelledGoals);

    ss << "  \"goalStack\": [\n";
    for (size_t i = 0; i < displayActiveGoals.size(); ++i) {
        ss << "    " << jsonGoalEntry(displayActiveGoals[i]);
        if (i + 1 < displayActiveGoals.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"completedGoals\": [\n";
    for (size_t i = 0; i < displayCompletedGoals.size(); ++i) {
        ss << "    " << jsonGoalEntry(displayCompletedGoals[i]);
        if (i + 1 < displayCompletedGoals.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"cancelledGoals\": [\n";
    for (size_t i = 0; i < displayCancelledGoals.size(); ++i) {
        ss << "    " << jsonGoalEntry(displayCancelledGoals[i]);
        if (i + 1 < displayCancelledGoals.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"astarSuccessors\": [\n";
    for (size_t i = 0; i < filtered.size(); ++i) {
        bool chosen = (filtered[i].pos == trace.chosenNode);
        ss << "    " << jsonNode(filtered[i], chosen);
        if (i+1 < filtered.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";

    const Node* cn = nullptr;
    for (const auto& n : filtered) {
        if (n.pos == trace.chosenNode) { cn = &n; break; }
    }
    if (!cn) {
        for (const auto& n : successors) {
            if (n.pos == trace.chosenNode) { cn = &n; break; }
        }
    }
    ss << "  \"chosenNode\": {\n";
    ss << "    \"label\": " << jsonString(positionToString(trace.chosenNode)) << ",\n";
    ss << "    \"pos\": "   << jsonPos(trace.chosenNode) << ",\n";
    ss << "    \"g\": "     << (cn ? cn->g : 0) << ",\n";
    ss << "    \"h\": "     << (cn ? cn->h : 0) << ",\n";
    ss << "    \"f\": "     << (cn ? cn->g + cn->h : 0) << "\n";
    ss << "  },\n";
    ss << "  \"activeGoalType\": " << jsonString(activeGoalTypeStr) << "\n";
    ss << "}\n";

    return ss.str();}
