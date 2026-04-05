#include "GridDisplay.h"
#include <iostream>
#include <string>

using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

void GridDisplay::clearScreen() {
    cout << "\033[2J\033[1;1H"; 
}

void GridDisplay::print(const GameEngine& engine) {
    clearScreen();
    const Grid& grid = engine.grid;
    int curZ = engine.robber.pos.z;

    cout << BOLD << CYAN << "================= MONEY HEIST 3D =================\n" << RESET;
    cout << YELLOW << engine.gameStatusMessage << "\n" << RESET;
    cout << "Turn: " << BOLD << engine.turnNumber << RESET;
    cout << " | Vault Collected: " << (engine.robber.hasVault ? (GREEN + string("YES") + RESET) : (RED + string("NO") + RESET));
    cout << " | Floor: " << (curZ + 1) << "\n\n";

    for (int y = 0; y < grid.getHeight(); ++y) {
        string mainLine = "";
        for (int x = 0; x < grid.getWidth(); ++x) {
            Position currentPos = {x, y, curZ};
            
            bool hasRobber = (engine.robber.pos == currentPos);
            bool hasPolice = false;
            for(const auto& p : engine.policeList) if(p.getPosition() == currentPos) hasPolice = true;
            bool hasCCTV = false;
            for(const auto& c : engine.cctvList) if(c.getPosition() == currentPos) hasCCTV = true;
            
            bool isVault = (grid.vaultPos == currentPos && !engine.robber.hasVault);
            bool isExit = (grid.exitPos == currentPos);
            
            char cellChar = grid.getCell(x, y, curZ);
            
            if (hasRobber) {
                mainLine += BOLD + string(GREEN) + " R" + RESET;
            } else if (hasPolice) {
                mainLine += BOLD + string(RED) + " P" + RESET;
            } else if (hasCCTV) {
                mainLine += BOLD + string(MAGENTA) + " C" + RESET;
            } else if (isVault) {
                mainLine += BOLD + string(YELLOW) + "$$" + RESET;
            } else if (isExit) {
                mainLine += BOLD + string(BLUE) + "[]" + RESET;
            } else if (cellChar == '#') {
                mainLine += BOLD + string("##") + RESET;
            } else if (cellChar == 'U') {
                mainLine += YELLOW + string("^^") + RESET;
            } else if (cellChar == 'D') {
                mainLine += YELLOW + string("vv") + RESET;
            } else {
                mainLine += " .";
            }
        }
        
        string miniMapStr = "       ";
        int miniZ = (curZ + 1 < grid.getDepth()) ? curZ + 1 : (curZ - 1 >= 0 ? curZ - 1 : curZ);
        if (miniZ != curZ && grid.getDepth() > 1) {
            if (y == 0) miniMapStr += BOLD + string("Minimap (Floor ") + to_string(miniZ + 1) + "):" + RESET;
            else if (y - 1 < grid.getHeight()) {
                int my = y - 1;
                for (int mx = 0; mx < grid.getWidth(); ++mx) {
                    Position currentPos = {mx, my, miniZ};
                    bool hasPolice = false;
                    for(const auto& p : engine.policeList) if(p.getPosition() == currentPos) hasPolice = true;
                    
                    if (hasPolice) miniMapStr += RED + string("P") + RESET;
                    else if (grid.getCell(mx, my, miniZ) == '#') miniMapStr += "X";
                    else if (grid.getCell(mx, my, miniZ) == 'U') miniMapStr += "^";
                    else if (grid.getCell(mx, my, miniZ) == 'D') miniMapStr += "v";
                    else miniMapStr += " ";
                }
            }
        }

        cout << mainLine << miniMapStr << "\n";
    }

    cout << "\n" << BOLD << "Legend:   " << RESET << "[$$] Money/Vault | [[]] Exit | [^^] Stairs Up | [vv] Stairs Down | [C] CCTV\n";
    cout << BOLD << "Controls: " << RESET << "[W/A/S/D] Move   | [U] Up Stairs | [J] Down Stairs | [Q] Quit\n\n";
    
    if (!engine.robber.hasVault && grid.vaultPos.z != curZ) {
        cout << YELLOW << "-> Tip: The $$ MONEY $$ is located on Floor " << grid.vaultPos.z + 1 << "!\n" << RESET;
    } else if (engine.robber.hasVault && grid.exitPos.z != curZ) {
        cout << BLUE << "-> Tip: The EXIT [] is located on Floor " << grid.exitPos.z + 1 << "!\n" << RESET;
    }
    
    cout << "--------------------------------------------------\n";
}
