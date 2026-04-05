#include <iostream>
#include "GameEngine.h"

using namespace std;

int main() {
    cout << "\033[2J\033[1;1H";
    cout << "\033[1;36m";
    cout << "==========================================\n";
    cout << "             MONEY HEIST 3D               \n";
    cout << "==========================================\n\n";
    cout << "\033[0m";
    
    cout << "Welcome to the Vault Setup.\n";
    cout << "Select Difficulty Level:\n";
    cout << "1. \033[32mEasy\033[0m   (1 Floor, 1 Static Guard)\n";
    cout << "2. \033[33mNormal\033[0m (2 Floors, 2 Police, 2 CCTVs)\n";
    cout << "3. \033[31mHard\033[0m   (3 Floors, 3 Police, 3 CCTVs)\n";
    cout << "\nEnter your choice (1-3): ";
    
    int diff;
    if (!(cin >> diff)) diff = 1;
    if (diff < 1 || diff > 3) diff = 1;

    GameEngine engine;
    engine.setDifficulty(diff);
    engine.run();

    return 0;
}