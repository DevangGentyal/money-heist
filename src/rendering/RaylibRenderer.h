#ifndef RAYLIB_RENDERER_H
#define RAYLIB_RENDERER_H

#include "raylib.h"
#include "../grid/Grid3D.h"
#include <string>
#include <vector>

using namespace std;

class RaylibRenderer {
private:
    const Grid3D& grid;
    int windowWidth;
    int windowHeight;
    int cellSize;
    int gridStartX;
    int gridStartY;
    int currentFloor;
    int turn;
    string status;
    bool vaultCollected;
    
    // Colors
    struct Colors {
        unsigned int wall;
        unsigned int empty;
        unsigned int robber;
        unsigned int police;
        unsigned int vault;
        unsigned int exit;
        unsigned int cctv;
        unsigned int alert;
        unsigned int stairs;
        unsigned int elevator;
        unsigned int background;
        unsigned int text;
        unsigned int grid;
    };
    Colors colors;
    
public:
    RaylibRenderer(const Grid3D& g);
    ~RaylibRenderer();
    
    // Initialize Raylib window
    void initWindow();
    void closeWindow();
    
    // Main render function
    void render(const Position& robberPos,
               const vector<Position>& policePositions,
               int turnCount,
               const string& statusMsg,
               bool hasVaultLoot,
               bool showPopup,
               const string& popupText,
               bool showGameOver,
               const string& gameOverTitle,
               const string& gameOverPrompt);
    
    // Render components
    void renderGrid(const Position& robberPos,
                   const vector<Position>& policePositions);
    void renderCell(int x, int y, const Position& robberPos,
                   const vector<Position>& policePositions);
    void renderHUD();
    void renderFloorIndicator();
    void renderMiniMaps(const Position& robberPos,
                       const vector<Position>& policePositions);
    void renderLegend();
    void renderControlsPanel();
    
    // Utilities
    bool isWindowOpen() const;
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    int getCellSize() const { return cellSize; }
    int getGridStartX() const { return gridStartX; }
    int getGridStartY() const { return gridStartY; }
    void setCurrentFloor(int z) { currentFloor = z; }
    
    // Draw utilities
    void drawCell(int screenX, int screenY, unsigned int color, const string& icon = "");
    void drawAgent(int screenX, int screenY, bool isRobber);
    void drawText(const string& text, int x, int y, int fontSize, unsigned int color);
    void drawCenteredText(const string& text, int x, int y, int width, int height, int fontSize, Color color);
    void drawPopup(const string& text);
    void drawGameOverOverlay(const string& title, const string& prompt);
};

#endif
