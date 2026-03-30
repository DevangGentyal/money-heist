#include "GameEngine.h"
#include "GridDisplay.h"
#include <iostream>

using namespace std;

GameEngine::GameEngine()
    : robber(grid.getInitialRobberPos()), police(grid.getInitialPolicePos()),
      turnNumber(1), gameRunning(true) {}

void GameEngine::run() {
  while (gameRunning) {
    processTurn();
  }
}

void GameEngine::processTurn() {
  GridDisplay::print(grid, robber.getPosition(), police.getPosition(),
                     robber.hasVault(), turnNumber);

  char move;
  bool validMove = false;
  while (!validMove) {
    cout << "Your move (w/a/s/d): ";
    cin >> move;
    validMove = robber.move(move, grid);
  }

  // Check if robber collected vault
  if (robber.getPosition() == grid.getVaultPos() && !robber.hasVault()) {
    robber.collectVault();
    cout << "Vault collected!\n";
  }

  // Check win condition after robber move
  checkWinConditions();
  if (!gameRunning)
    return;

  // Police move
  police.moveTowards(robber.getPosition(), grid, robber.hasVault(), turnNumber);

  // Check win condition after police move
  checkWinConditions();

  turnNumber++;
}

void GameEngine::checkWinConditions() {
  Position robberPos = robber.getPosition();
  Position policePos = police.getPosition();

  // Police wins
  if (robberPos == policePos) {
    GridDisplay::print(grid, robberPos, policePos, robber.hasVault(),
                       turnNumber);
    cout << "\nGAME OVER: The Police caught the Robber!\n";
    gameRunning = false;
    return;
  }

  // Robber wins
  if (robber.hasVault() && robberPos == grid.getExitPos()) {
    GridDisplay::print(grid, robberPos, policePos, robber.hasVault(),
                       turnNumber);
    cout << "\nCONGRATULATIONS: The Robber escaped with the loot!\n";
    gameRunning = false;
    return;
  }
}
