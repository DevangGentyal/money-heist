#include "core/GameEngineGUI.h"
#include <iostream>
#include <cstdlib>

using namespace std;

int selectDifficulty() {
    int choice;
    cout << "\n\033[1mSelect Difficulty Level:\033[0m\n";
    cout << "1. \033[32mEasy\033[0m   (1 Floor, 1 Police, Single-floor heist)\n";
    cout << "2. \033[33mNormal\033[0m (2 Floors, 2 Police, Vertical strategy needed)\n";
    cout << "3. \033[31mHard\033[0m   (3 Floors, 3 Police, Complex multi-floor planning)\n";
    cout << "\nEnter choice (1-3): ";
    
    cin >> choice;
    if (choice < 1 || choice > 3) choice = 1;
    
    return choice;
}

PlayerRole selectRole() {
    cout << "\n\033[1mPlayer Role:\033[0m\n";
    cout << "1. \033[32mRobber\033[0m - Navigate to vault, grab loot, escape (Manual)\n";
    cout << "\nNote: Police mode is currently disabled (AI-only)\n";
    
    return PlayerRole::ROBBER;
}

void printBanner() {
    cout << "\033[2J\033[1;1H"; // Clear screen
    cout << "\033[1;36m"; // Cyan + Bold
    cout << "╔══════════════════════════════════════╗\n";
    cout << "║   MONEY HEIST 3D: GUI EDITION       ║\n";
    cout << "║  Adaptive Heuristic AI Simulator     ║\n";
    cout << "║  Powered by Raylib Graphics          ║\n";
    cout << "╚══════════════════════════════════════╝\n";
    cout << "\033[0m"; // Reset
#ifdef HEIST_GUI_BUILD_TAG
    cout << "Build: " << HEIST_GUI_BUILD_TAG << "\n";
#endif
}

int main() {
    printBanner();
    
    PlayerRole role = selectRole();
    int difficulty = selectDifficulty();
    
    cout << "\n\033[1;33m📌 KEY FEATURES:\033[0m\n";
    cout << "  ✓ 3D Grid: Multi-floor heist with vertical movement\n";
    cout << "  ✓ Custom Heuristic: Weighted scoring for safety/distance\n";
    cout << "  ✓ Predictive AI: Police predicts robber path and intercepts\n";
    cout << "  ✓ Dynamic Difficulty: CCTV/Alert zones trigger difficulty spikes\n";
    cout << "  ✓ Modern GUI: Raylib-powered graphics rendering\n";
    cout << "\nLaunching GUI window...\n\n";
    
    GameEngineGUI engine;
    engine.initialize(difficulty, role);
    engine.run();
    
    cout << "\n\033[1;36m" << engine.getStatusMessage() << "\033[0m\n";
    cout << "Thanks for playing Money Heist 3D GUI Edition!\n";
    
    return 0;
}
