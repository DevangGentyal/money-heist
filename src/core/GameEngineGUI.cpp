#include "GameEngineGUI.h"
#include "raylib.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <iostream>

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
    Position down(current.x, current.y, current.z - 1);
    if (grid.isValid(down) && !grid.isWall(down)) {
      robber.setPosition(down);
    }
  }
}
} // namespace

GameEngineGUI::GameEngineGUI()
    : controlledPoliceIndex(0), difficulty(1), turnCount(0),
      gameStatus(GameStatus::RUNNING), vaultCollected(false),
      vaultPopupFrames(0), policeBoostTurnsRemaining(0),
      policeDoubleStepActive(false), hoveredCell(0, 0, 0),
      hasHoveredCell(false) {}

GameEngineGUI::~GameEngineGUI() {}

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
    mapDifficulty = 4 - difficulty; // 1->3, 2->2, 3->1
  }
  grid.loadLevel(mapDifficulty);

  // Initialize AI engines
  heuristic = make_unique<HeuristicEngine>(grid);
  heuristic->adjustForDifficulty(difficulty);

  predictor = make_unique<PredictionEngine>(grid, 10 + difficulty * 2);

  rules = make_unique<RuleEngine>(difficulty);

  // Initialize robber
  robber = make_unique<RobberAI>(grid.getInitialRobberPos(), heuristic.get());

  // Initialize police strictly from hardcoded 'P' positions in Grid3D maps.
  vector<Position> hardcodedPoliceSpawns = grid.getInitialPolicePos();
  if (hardcodedPoliceSpawns.empty()) {
    // Deterministic emergency fallback if a map is missing P markers.
    hardcodedPoliceSpawns.push_back(Position(1, 1, 0));
  }

  for (const auto &policeStart : hardcodedPoliceSpawns) {
    auto police = make_unique<PoliceAI>(policeStart, predictor.get(),
                                        heuristic.get(), rules.get());
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
  vector<Position> policePositions;
  for (const auto &p : policeList) {
    policePositions.push_back(p->getPosition());
  }

  bool showGameOver = gameStatus == GameStatus::ROBBER_WON ||
                      gameStatus == GameStatus::ROBBER_ESCAPED_NO_VAULT ||
                      gameStatus == GameStatus::POLICE_WON;
  string gameOverTitle = "";
  if (gameStatus == GameStatus::ROBBER_WON) {
    gameOverTitle = "ROBBER VICTORY";
  } else if (gameStatus == GameStatus::ROBBER_ESCAPED_NO_VAULT) {
    gameOverTitle = "ROBBER ESCAPED";
  } else if (gameStatus == GameStatus::POLICE_WON) {
    gameOverTitle = "POLICE VICTORY";
  }

  renderer->render(robber->getPosition(), policePositions, turnCount,
                   statusMessage, vaultCollected, vaultPopupFrames > 0,
                   popupMessage, showGameOver, gameOverTitle,
                   "Press R to Retry or ESC to Quit");
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

  if (robber->hasReachedExit(grid) && vaultCollected) {
    gameStatus = GameStatus::ROBBER_WON;
    statusMessage = "Robber Escaped with Vault";
    return true;
  }

  if (robber->hasReachedExit(grid) && !vaultCollected) {
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

  Position robberMove = robber->getNextMove(grid, policePositions);
  if (robber->canMoveTo(robberMove, grid)) {
    robber->setPosition(robberMove);
    applyRobberFloorTransition(*robber, grid);
    refreshVaultCollectionState();

    // If AI robber steps on alert, police boost should unlock immediately.
    if (grid.isAlertZone(robber->getPosition())) {
      policeBoostTurnsRemaining = 5;
      policeDoubleStepActive = true;
      grid.setCell(robber->getPosition(), CellType::CCTV_ZONE);
      popupMessage = "Alert Triggered!";
      vaultPopupFrames = 120;
      statusMessage = "🚨 ALERT ZONE! Police move x2";
    }
  }

  syncControlledPoliceToRobberFloor();

  if (robber->hasReachedExit(grid) && vaultCollected) {
    gameStatus = GameStatus::ROBBER_WON;
    statusMessage = "Robber Escaped with Vault";
    return false;
  }

  if (robber->hasReachedExit(grid) && !vaultCollected) {
    gameStatus = GameStatus::ROBBER_ESCAPED_NO_VAULT;
    statusMessage = "Robber Escaped without Vault";
    return false;
  }

  for (const auto &police : policeList) {
    if (police->getPosition() == robber->getPosition()) {
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

  int robberFloor = robber->getPosition().z;

  if (controlledPoliceIndex >= 0 &&
      controlledPoliceIndex < static_cast<int>(policeList.size()) &&
      policeList[controlledPoliceIndex]->getPosition().z == robberFloor) {
    return;
  }

  int bestIndex = -1;
  int bestDistance = std::numeric_limits<int>::max();
  Position robberPos = robber->getPosition();

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
    return robber->canMoveTo(targetPos, grid);
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
    return robber->getPosition();
  }

  if (policeList.empty() || controlledPoliceIndex < 0 ||
      controlledPoliceIndex >= static_cast<int>(policeList.size())) {
    return Position(0, 0, 0);
  }

  return policeList[controlledPoliceIndex]->getPosition();
}

void GameEngineGUI::setPlayerPosition(const Position &newPos) {
  if (playerRole == PlayerRole::ROBBER) {
    robber->setPosition(newPos);

    Position updatedPos = robber->getPosition();
    CellType atCell = grid.getCell(updatedPos);
    if (atCell == CellType::STAIRS) {
      Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
      if (grid.isValid(up) && !grid.isWall(up) && !isPoliceOnCell(up)) {
        robber->setPosition(up);
      } else if (isPoliceOnCell(up)) {
        statusMessage = "Blocked: police at upstairs landing";
      }
    } else if (atCell == CellType::ELEVATOR) {
      Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
      if (grid.isValid(down) && !grid.isWall(down) && !isPoliceOnCell(down)) {
        robber->setPosition(down);
      } else if (isPoliceOnCell(down)) {
        statusMessage = "Blocked: police at downstairs landing";
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
        }
      } else if (atCell == CellType::ELEVATOR) {
        Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
        if (grid.isValid(down) && !grid.isWall(down)) {
          policeList[controlledPoliceIndex]->setPosition(down);
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
  } else if (cellType == CellType::ELEVATOR) {
    return true;
  }

  return false;
}

void GameEngineGUI::updateAI() {
  policeDoubleStepActive = policeBoostTurnsRemaining > 0;

  vector<Position> policePositions;
  for (const auto &p : policeList) {
    policePositions.push_back(p->getPosition());
  }

  if (playerRole == PlayerRole::ROBBER) {
    if (grid.isAlertZone(robber->getPosition())) {
      policeBoostTurnsRemaining = 5;
      grid.setCell(robber->getPosition(), CellType::CCTV_ZONE);
      popupMessage = "Alert Triggered!";
      vaultPopupFrames = 120;
    }

    int policeSteps = policeDoubleStepActive ? 2 : 1;

    set<Position> occupiedPolicePositions;
    for (const auto &police : policeList) {
      occupiedPolicePositions.insert(police->getPosition());
    }

    for (size_t i = 0; i < policeList.size(); ++i) {
      auto &police = policeList[i];
      occupiedPolicePositions.erase(police->getPosition());

      Position pursuitTarget = robber->getPosition();
      police->setVaultCollected(vaultCollected);

      // Get standard next move (direction candidate)
      vector<Position> stepTarget{pursuitTarget};
      Position dirMove = police->getNextMove(grid, stepTarget);

      if (dirMove == police->getPosition()) {
        occupiedPolicePositions.insert(police->getPosition());
        continue;
      }

      int dx = dirMove.x - police->getPosition().x;
      int dy = dirMove.y - police->getPosition().y;
      // No floor changes allowed in mid-turn linear boost

      Position step1 = dirMove;
      Position step2(step1.x + dx, step1.y + dy, step1.z);

      Position chosenMove = police->getPosition();

      // Debugging output
      std::cout << "[DEBUG] Police " << i
                << " evaluating linear moves in direction (" << dx << "," << dy
                << "):" << std::endl;

      // Catch check immediate
      if (policeSteps > 1 && step2 == pursuitTarget &&
          police->canMoveTo(step2, grid)) {
        chosenMove = step2;
        std::cout << "  - Catch at Step 2!" << std::endl;
      } else if (step1 == pursuitTarget && police->canMoveTo(step1, grid)) {
        chosenMove = step1;
        std::cout << "  - Catch at Step 1!" << std::endl;
      } else {
        float bestDist = static_cast<float>(
            grid.manhattanDistance(police->getPosition(), pursuitTarget));

        // Evaluate Step 1
        if (police->canMoveTo(step1, grid) &&
            !occupiedPolicePositions.count(step1)) {
          float d1 =
              static_cast<float>(grid.manhattanDistance(step1, pursuitTarget));
          std::cout << "  - Step 1 (" << step1.x << "," << step1.y << "): Dist "
                    << d1;
          if (d1 < bestDist) {
            bestDist = d1;
            chosenMove = step1;
          }
        }

        // Evaluate Step 2 if possible
        if (policeSteps > 1 && police->canMoveTo(step1, grid) &&
            police->canMoveTo(step2, grid) &&
            !occupiedPolicePositions.count(step2)) {
          float d2 =
              static_cast<float>(grid.manhattanDistance(step2, pursuitTarget));
          std::cout << "  - Step 2 (" << step2.x << "," << step2.y << "): Dist "
                    << d2;
          if (d2 < bestDist) {
            bestDist = d2;
            chosenMove = step2;
          }
        }
        std::cout << std::endl;
      }

      if (chosenMove != police->getPosition()) {
        police->setPosition(chosenMove);
        std::cout << "  - Final Move: (" << chosenMove.x << "," << chosenMove.y
                  << ")" << std::endl;

        if (shouldPoliceTransitionFloors(police->getPosition())) {
          Position updatedPos = police->getPosition();
          CellType atCell = grid.getCell(updatedPos);
          if (atCell == CellType::STAIRS) {
            Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
            if (grid.isValid(up) && !grid.isWall(up)) {
              police->setPosition(up);
            }
          } else if (atCell == CellType::ELEVATOR) {
            Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
            if (grid.isValid(down) && !grid.isWall(down)) {
              police->setPosition(down);
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
      vector<Position> robberAsTarget{robber->getPosition()};

      // Tell police about vault status so they know if they can use stairs
      policeList[i]->setVaultCollected(vaultCollected);
      Position policeMove = policeList[i]->getNextMove(grid, robberAsTarget);

      if (occupiedPolicePositions.count(policeMove) &&
          policeMove != robber->getPosition()) {
        continue;
      }

      if (policeList[i]->canMoveTo(policeMove, grid)) {
        policeList[i]->setPosition(policeMove);

        if (shouldPoliceTransitionFloors(policeList[i]->getPosition())) {
          Position updatedPos = policeList[i]->getPosition();
          CellType atCell = grid.getCell(updatedPos);
          if (atCell == CellType::STAIRS) {
            Position up(updatedPos.x, updatedPos.y, updatedPos.z + 1);
            if (grid.isValid(up) && !grid.isWall(up)) {
              policeList[i]->setPosition(up);
            }
          } else if (atCell == CellType::ELEVATOR) {
            Position down(updatedPos.x, updatedPos.y, updatedPos.z - 1);
            if (grid.isValid(down) && !grid.isWall(down)) {
              policeList[i]->setPosition(down);
            }
          }
        }

        occupiedPolicePositions.insert(policeList[i]->getPosition());
      }
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
  ctx.alertTriggered = policeDoubleStepActive;
  ctx.turnsInAlert = policeBoostTurnsRemaining;
  ctx.policeSpeed = policeDoubleStepActive ? 2.0f : 1.0f;
  ctx.predictionDepth = 10 + difficulty * 2;
  ctx.turnsAtLarge = turnCount;

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
  if (robber->hasReachedExit(grid) && vaultCollected) {
    gameStatus = GameStatus::ROBBER_WON;
    statusMessage = "Robber Escaped with Vault";
    return;
  }

  if (robber->hasReachedExit(grid) && !vaultCollected) {
    gameStatus = GameStatus::ROBBER_ESCAPED_NO_VAULT;
    statusMessage = "Robber Escaped without Vault";
    return;
  }

  // Police wins if catches robber
  for (const auto &police : policeList) {
    if (police->getPosition() == robber->getPosition()) {
      gameStatus = GameStatus::POLICE_WON;
      statusMessage = "Police Caught Robber";
      return;
    }
  }
}

void GameEngineGUI::refreshVaultCollectionState() {
  if (!vaultCollected && robber->getPosition() == grid.getVaultPos()) {
    vaultCollected = true;
    popupMessage = "Vault Collected!";
    vaultPopupFrames = 150;
    statusMessage = "💰 Vault secured - Reach EXIT";
  }
}
