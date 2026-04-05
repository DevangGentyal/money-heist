#include "GameEngine.h"
#include "GridDisplay.h"
#include <iostream>

using namespace std;

GameEngine::GameEngine() : turnNumber(0), gameRunning(true), difficulty(1), systemAlerted(false), alertPos({0,0,0}), gameStatusMessage("Game Started!") {}

void GameEngine::setDifficulty(int diff) {
    difficulty = diff;
    grid.loadLevel(difficulty);
    robber = Robber(grid.initialRobberPos);
    
    policeList.clear();
    for (Position p : grid.initialPolicePos) {
        policeList.push_back(PoliceAI(p));
    }
    
    cctvList.clear();
    for (Position p : grid.initialCCTVPos) {
        cctvList.push_back(CCTV(p));
    }
}

void GameEngine::run() {
    GridDisplay::clearScreen();
    while (gameRunning) {
        GridDisplay::print(*this);
        char input;
        cin >> input;
        if (input == 'q') {
            gameRunning = false;
            cout << "\033[33mYou voluntarily aborted the heist.\033[0m\n";
            break;
        }
        processTurn(input);
    }
    
    // Final game over screen
    GridDisplay::print(*this);
    cout << "\n";
    if (gameStatusMessage.find("Win") != string::npos) {
        cout << "\033[32m\033[1m";
        cout << "***************************************************\n";
        cout << "*                                                 *\n";
        cout << "*               MISSION ACCOMPLISHED              *\n";
        cout << "*        YOU ESCAPED WITH THE VAULT SECURED!      *\n";
        cout << "*                                                 *\n";
        cout << "***************************************************\n";
        cout << "\033[0m\n";
    } else if (gameStatusMessage.find("Lose") != string::npos) {
        cout << "\033[31m\033[1m";
        cout << "***************************************************\n";
        cout << "*                                                 *\n";
        cout << "*                 MISSION FAILED                  *\n";
        cout << "*       YOU WERE APPREHENDED BY THE AI POLICE     *\n";
        cout << "*                                                 *\n";
        cout << "***************************************************\n";
        cout << "\033[0m\n";
    }
}

void GameEngine::processTurn(char input) {
    if (!robber.move(input, grid)) {
        gameStatusMessage = "Invalid move! Hit a wall or couldn't use stairs.";
        return; 
    }
    
    turnNumber++;
    gameStatusMessage = "Robber Moved.";
    
    if (grid.vaultPos == robber.pos && !robber.hasVault) {
        robber.hasVault = true;
        gameStatusMessage = "Vault Collected! Head to the nearest EXIT (E)!";
    }
    
    checkWinConditions();
    if (!gameRunning) return;

    systemAlerted = false;
    for (auto& cctv : cctvList) {
        cctv.updateTurn();
        if (cctv.isSpotted(robber.pos, grid)) {
            systemAlerted = true;
            alertPos = robber.pos;
            gameStatusMessage = "⚠️ CCTV ALERT! Police dispatching to your location! ⚠️";
        }
    }
    
    for (auto& police : policeList) {
        int moves = (systemAlerted || police.state == AIState::ALERT) ? 2 : 1;
        for (int i=0; i<moves; i++) {
            police.takeTurn(robber.pos, systemAlerted, alertPos, grid);
            checkWinConditions();
            if (!gameRunning) {
                GridDisplay::print(*this);
                return;
            }
        }
    }
}

void GameEngine::checkWinConditions() {
    if (robber.pos == grid.exitPos && robber.hasVault) {
        gameStatusMessage = "🎉 YOU ESCAPED! You Win! 🎉";
        gameRunning = false;
        return;
    }
    
    for (const auto& police : policeList) {
        if (robber.pos == police.getPosition()) {
            gameStatusMessage = "❌ CAUGHT! Or BUSTED! You Lose! ❌";
            gameRunning = false;
            return;
        }
    }
}
