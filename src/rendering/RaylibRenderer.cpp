#include "RaylibRenderer.h"
#include "../BuildInfo.h"
#include "raylib.h"
#include <cmath>
#include <sstream>
#include <algorithm>

RaylibRenderer::RaylibRenderer(const Grid3D& g)
    : grid(g), windowWidth(1400), windowHeight(900),
    cellSize(0), gridStartX(20), gridStartY(20), currentFloor(0), turn(0), status("Initializing..."), vaultCollected(false) {
    
    // Calculate initial cell size based on both width and height limits.
    int widthLimitedCell = (windowWidth - 420) / grid.getWidth();
    int heightLimitedCell = (windowHeight - 40) / grid.getHeight();
    cellSize = std::max(20, std::min(widthLimitedCell, heightLimitedCell));
    
    // Initialize colors
    colors.wall = 0x000000FF;
    colors.empty = 0x163A5BFF;
    colors.robber = 0x2ECC71FF;
    colors.police = 0xE74C3CFF;
    colors.vault = 0xF39C12FF;
    colors.exit = 0x3498DBFF;
    colors.cctv = 0x9B59B6FF;
    colors.alert = 0xE67E22FF;
    colors.stairs = 0x1ABC9CFF;
    colors.elevator = 0x16A085FF;
    colors.background = 0x1A1A2EFF;
    colors.text = 0xECF0F1FF;
    colors.grid = 0xA9A9A9FF;
}

RaylibRenderer::~RaylibRenderer() {
    closeWindow();
}

void RaylibRenderer::initWindow() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(windowWidth, windowHeight, "Money Heist 3D: GUI Edition");
    SetWindowMinSize(1320, 900);
    SetTargetFPS(60);
}

void RaylibRenderer::closeWindow() {
    CloseWindow();
}

bool RaylibRenderer::isWindowOpen() const {
    return !WindowShouldClose();
}

void RaylibRenderer::render(const Position& robberPos,
                           const vector<Position>& policePositions,
                           int turnCount,
                           const string& statusMsg,
                           bool hasVaultLoot,
                           bool showPopup,
                           const string& popupText,
                           bool showGameOver,
                           const string& gameOverTitle,
                           const string& gameOverPrompt) {
    turn = turnCount;
    status = statusMsg;
    currentFloor = robberPos.z;
    vaultCollected = hasVaultLoot;

    windowWidth = GetScreenWidth();
    windowHeight = GetScreenHeight();
    int widthLimitedCell = (windowWidth - 600) / grid.getWidth();
    int heightLimitedCell = (windowHeight - 80) / grid.getHeight();
    cellSize = std::max(20, std::min(widthLimitedCell, heightLimitedCell));
    gridStartX = 20;
    gridStartY = std::max(20, (windowHeight - (grid.getHeight() * cellSize)) / 2);
    
    BeginDrawing();
    ClearBackground({26, 26, 46, 255});
    
    // Main game area
    renderGrid(robberPos, policePositions);
    
    // Right sidebar
    renderFloorIndicator();
    renderHUD();
    renderMiniMaps(robberPos, policePositions);
    renderLegend();
    renderControlsPanel();

    if (showPopup) {
        drawPopup(popupText);
    }

    if (showGameOver) {
        drawGameOverOverlay(gameOverTitle, gameOverPrompt);
    }
    
    EndDrawing();
}

void RaylibRenderer::renderGrid(const Position& robberPos,
                               const vector<Position>& policePositions) {
    // Draw grid info
    DrawText(TextFormat("FLOOR %d / %d", currentFloor + 1, grid.getDepth()),
             gridStartX, gridStartY - 30, 20, {236, 240, 241, 255});
    
    // Draw cells
    for (int y = 0; y < grid.getHeight(); y++) {
        for (int x = 0; x < grid.getWidth(); x++) {
            renderCell(x, y, robberPos, policePositions);
        }
    }
    
    // Draw grid border
    DrawRectangleLines(gridStartX - 2, gridStartY - 2,
                      grid.getWidth() * cellSize + 4,
                      grid.getHeight() * cellSize + 4,
                      {149, 165, 166, 255});
}

void RaylibRenderer::renderCell(int x, int y, const Position& robberPos,
                               const vector<Position>& policePositions) {
    int screenX = gridStartX + x * cellSize;
    int screenY = gridStartY + y * cellSize;
    
    Position cellPos(x, y, currentFloor);
    
    // Background color based on cell type
    Color cellColor;
    string icon = "";
    
    // Check agents: Police should take precedence over robber to show "catch" clearly
    for (const auto& p : policePositions) {
        if (cellPos == p) {
            drawAgent(screenX, screenY, false);
            return;
        }
    }

    if (cellPos == robberPos) {
        drawAgent(screenX, screenY, true);
        return;
    }
    
    // Cell type
    CellType type = grid.getCell(cellPos);
    switch (type) {
        case CellType::WALL:
            cellColor = GetColor(colors.wall);
            break;
        case CellType::VAULT:
            cellColor = vaultCollected ? Color{130, 130, 130, 255} : Color{243, 156, 18, 255};
            icon = "V";
            break;
        case CellType::EXIT:
            cellColor = {52, 152, 219, 255};
            icon = "E";
            break;
        case CellType::CCTV_ZONE:
            cellColor = {178, 34, 34, 255};
            icon = "!";
            break;
        case CellType::ALERT_ZONE:
            cellColor = {230, 126, 34, 255};
            icon = "A";
            break;
        case CellType::STAIRS:
            cellColor = {26, 188, 156, 255};
            icon = "US";
            break;
        case CellType::ELEVATOR:
            cellColor = {22, 160, 133, 255};
            icon = "DS";
            break;
        default:
            cellColor = GetColor(colors.empty);
    }
    
    drawCell(screenX, screenY, ColorToInt(cellColor), icon);

    Rectangle tileRect = { static_cast<float>(screenX), static_cast<float>(screenY), static_cast<float>(cellSize), static_cast<float>(cellSize) };

    if (type == CellType::EXIT) {
        DrawRectangleLinesEx(tileRect, 3.0f, Color{255, 255, 255, 255});
        DrawRectangleLinesEx(tileRect, 1.0f, Color{52, 152, 219, 255});
    } else if (type == CellType::STAIRS) {
        DrawRectangleLinesEx(tileRect, 3.0f, Color{160, 255, 235, 255});
    } else if (type == CellType::ELEVATOR) {
        DrawRectangleLinesEx(tileRect, 3.0f, Color{135, 220, 255, 255});
    } else if (type == CellType::VAULT && vaultCollected) {
        DrawRectangleLinesEx(tileRect, 2.0f, Color{190, 190, 190, 255});
    }
}

void RaylibRenderer::drawCell(int screenX, int screenY, unsigned int color,
                             const string& icon) {
    Color c = GetColor(color);
    DrawRectangle(screenX + 1, screenY + 1, cellSize - 2, cellSize - 2, c);
    DrawRectangleLines(screenX, screenY, cellSize, cellSize, GetColor(colors.grid));
    
    if (!icon.empty()) {
        int fontSize = cellSize / 3;
        drawCenteredText(icon, screenX, screenY, cellSize, cellSize, fontSize, WHITE);
    }
}

void RaylibRenderer::drawAgent(int screenX, int screenY, bool isRobber) {
    Color agentColor = isRobber ? 
        Color{46, 204, 113, 255} : Color{231, 76, 60, 255};
    
    int centerX = screenX + cellSize / 2;
    int centerY = screenY + cellSize / 2;
    int radius = cellSize / 2 - 2;
    
    DrawCircle(centerX, centerY, radius, agentColor);
    DrawCircleLines(centerX, centerY, radius, {0, 0, 0, 255});
    
    drawCenteredText(isRobber ? "R" : "P", screenX, screenY, cellSize, cellSize, cellSize / 3, BLACK);
}

void RaylibRenderer::renderHUD() {
    int hudX = gridStartX + grid.getWidth() * cellSize + 40;
    int hudY = 40;
    
    // Status box
    DrawRectangle(hudX, hudY, 520, 145, {44, 62, 80, 220});
    DrawRectangleLines(hudX, hudY, 520, 145, {236, 240, 241, 255});
    
    DrawText("=== GAME STATUS ===", hudX + 20, hudY + 15, 16, {236, 240, 241, 255});
    DrawText(TextFormat("Turn: %d", turn), hudX + 20, hudY + 45, 14, {236, 240, 241, 255});
    DrawText(TextFormat("Status: %s", status.c_str()), hudX + 20, hudY + 70, 13, {236, 240, 241, 255});
    
    // Difficulty info
    DrawText("Floor:", hudX + 20, hudY + 100, 14, {236, 240, 241, 255});
    DrawText(TextFormat("%d / %d", currentFloor + 1, grid.getDepth()), hudX + 150, hudY + 100, 14, {52, 152, 219, 255});
}

void RaylibRenderer::renderFloorIndicator() {
    int hudX = gridStartX + grid.getWidth() * cellSize + 40;
    int hudY = 200;
    
    DrawRectangle(hudX, hudY, 520, 130, {44, 62, 80, 220});
    DrawRectangleLines(hudX, hudY, 520, 130, {236, 240, 241, 255});
    
    DrawText("=== OBJECTIVE ===", hudX + 20, hudY + 15, 16, {236, 240, 241, 255});
    
    if (currentFloor == grid.getVaultPos().z) {
        DrawText("Position: VAULT FLOOR", hudX + 20, hudY + 45, 13, {243, 156, 18, 255});
    } else if (currentFloor == grid.getExitPos().z) {
        DrawText("Position: EXIT FLOOR", hudX + 20, hudY + 45, 13, {52, 152, 219, 255});
    } else {
        DrawText("Position: TRANSIT FLOOR", hudX + 20, hudY + 45, 13, {149, 165, 166, 255});
    }
    
    DrawText("ROBBER Goal: Vault -> Exit", hudX + 20, hudY + 74, 12, {46, 204, 113, 255});
    DrawText("POLICE Goal: Catch Robber", hudX + 20, hudY + 96, 12, {231, 76, 60, 255});
}

void RaylibRenderer::renderLegend() {
    int hudX = gridStartX + grid.getWidth() * cellSize + 40;
    int hudY = 730;
    
    DrawRectangle(hudX, hudY, 520, 90, {44, 62, 80, 220});
    DrawRectangleLines(hudX, hudY, 520, 90, {236, 240, 241, 255});
    
    DrawText("=== LEGEND ===", hudX + 20, hudY + 15, 16, {236, 240, 241, 255});
    
    int itemY = hudY + 42;
    
    // Draw legend items with colors
    DrawRectangle(hudX + 20, itemY, 15, 15, {46, 204, 113, 255});
    DrawText("R = Robber", hudX + 45, itemY, 12, {236, 240, 241, 255});
    
    itemY += 18;
    DrawRectangle(hudX + 20, itemY, 15, 15, {231, 76, 60, 255});
    DrawText("P = Police", hudX + 45, itemY, 12, {236, 240, 241, 255});
    
    itemY += 18;
    DrawRectangle(hudX + 20, itemY, 15, 15, {243, 156, 18, 255});
    DrawText("V = Vault", hudX + 45, itemY, 12, {236, 240, 241, 255});
    
    itemY += 18;
    DrawRectangle(hudX + 20, itemY, 15, 15, {52, 152, 219, 255});
    DrawText("E = Exit", hudX + 45, itemY, 12, {236, 240, 241, 255});
    
    DrawRectangle(hudX + 260, hudY + 42, 15, 15, {178, 34, 34, 255});
    DrawText("! = Alert", hudX + 285, hudY + 42, 12, {236, 240, 241, 255});

    DrawRectangle(hudX + 260, hudY + 60, 15, 15, {230, 126, 34, 255});
    DrawText("A = Zone", hudX + 285, hudY + 60, 12, {236, 240, 241, 255});

    DrawRectangle(hudX + 395, hudY + 42, 15, 15, {26, 188, 156, 255});
    DrawText("US/DS", hudX + 420, hudY + 42, 12, {236, 240, 241, 255});

    DrawRectangle(hudX + 395, hudY + 60, 15, 15, {22, 160, 133, 255});
    DrawText("Transitions", hudX + 420, hudY + 60, 12, {236, 240, 241, 255});
}

void RaylibRenderer::renderControlsPanel() {
    int hudX = gridStartX + grid.getWidth() * cellSize + 40;
    int hudY = 830;
    
    DrawRectangle(hudX, hudY, 520, 60, {44, 62, 80, 220});
    DrawRectangleLines(hudX, hudY, 520, 60, {236, 240, 241, 255});
    
    DrawText("WASD Move | Step on US/DS to change floor | ESC quit", hudX + 16, hudY + 10, 16, {236, 240, 241, 255});
    DrawText(TextFormat("Build [%s]: %s", heistBuildFlavor(), heistBuildTag()), hudX + 16, hudY + 34, 12, {255, 224, 130, 255});
}

void RaylibRenderer::renderMiniMaps(const Position& robberPos,
                                   const vector<Position>& policePositions) {
    if (grid.getDepth() <= 1) {
        return;
    }

    int hudX = gridStartX + grid.getWidth() * cellSize + 40;
    int hudY = 350;

    DrawRectangle(hudX, hudY, 520, 360, {44, 62, 80, 220});
    DrawRectangleLines(hudX, hudY, 520, 360, {236, 240, 241, 255});
    DrawText("=== MINI MAP ===", hudX + 20, hudY + 12, 16, {236, 240, 241, 255});

    vector<int> floorsToShow;
    if (currentFloor < grid.getDepth() - 1) {
        floorsToShow.push_back(currentFloor + 1);
    } else {
        for (int floor = currentFloor - 1; floor >= 0; --floor) {
            floorsToShow.push_back(floor);
        }
    }

    auto drawMiniFloor = [&](int floorToRender, int mapX, int mapY, int miniCell) {
        for (int y = 0; y < grid.getHeight(); ++y) {
            for (int x = 0; x < grid.getWidth(); ++x) {
                Position cellPos(x, y, floorToRender);
                Color cellColor = Color{22, 58, 91, 255};
                Color textColor = WHITE;
                string marker;

                if (grid.isWall(cellPos)) {
                    cellColor = Color{0, 0, 0, 255};
                    textColor = Color{180, 180, 180, 255};
                } else {
                    CellType t = grid.getCell(cellPos);
                    if (t == CellType::VAULT) {
                        cellColor = vaultCollected ? Color{110, 110, 110, 255} : Color{243, 156, 18, 255};
                        marker = "V";
                    } else if (t == CellType::EXIT) {
                        cellColor = Color{52, 152, 219, 255};
                        marker = "E";
                    } else if (t == CellType::STAIRS) {
                        cellColor = Color{26, 188, 156, 255};
                        marker = "U";
                    } else if (t == CellType::ELEVATOR) {
                        cellColor = Color{22, 160, 133, 255};
                        marker = "D";
                    } else if (t == CellType::CCTV_ZONE) {
                        cellColor = Color{178, 34, 34, 255};
                        marker = "!";
                    } else if (t == CellType::ALERT_ZONE) {
                        cellColor = Color{230, 126, 34, 255};
                        marker = "A";
                    }
                }

                if (cellPos == robberPos) {
                    cellColor = Color{46, 204, 113, 255};
                    marker = "R";
                    textColor = BLACK;
                } else {
                    for (const auto& policePos : policePositions) {
                        if (cellPos == policePos) {
                            cellColor = Color{231, 76, 60, 255};
                            marker = "P";
                            textColor = WHITE;
                            break;
                        }
                    }
                }

                int cellX = mapX + x * miniCell;
                int cellY = mapY + y * miniCell;
                DrawRectangle(cellX, cellY, miniCell, miniCell, cellColor);
                DrawRectangleLines(cellX, cellY, miniCell, miniCell, Color{34, 44, 58, 255});

                if (!marker.empty() && miniCell >= 10) {
                    int markerSize = miniCell - 4;
                    drawCenteredText(marker, cellX, cellY, miniCell, miniCell, markerSize, textColor);
                }
            }
        }
    };

    if (floorsToShow.size() == 1) {
        int miniCell = 18;
        int mapWidth = grid.getWidth() * miniCell;
        int mapHeight = grid.getHeight() * miniCell;
        int startX = hudX + (520 - mapWidth) / 2;
        int startY = hudY + 46;
        int floorToRender = floorsToShow[0];
        DrawText(TextFormat("Floor %d (adjacent live)", floorToRender + 1), startX, startY - 20, 14, {200, 220, 255, 255});
        drawMiniFloor(floorToRender, startX, startY, miniCell);
        DrawRectangleLines(startX - 1, startY - 1, mapWidth + 2, mapHeight + 2, Color{170, 185, 200, 255});
        return;
    }

    int miniCell = 12;
    int mapWidth = grid.getWidth() * miniCell;
    int mapHeight = grid.getHeight() * miniCell;
    int gap = 20;
    int startY = hudY + 58;
    int startXLeft = hudX + (520 - (mapWidth * 2 + gap)) / 2;

    for (size_t i = 0; i < floorsToShow.size() && i < 2; ++i) {
        int floorToRender = floorsToShow[i];
        int mapX = startXLeft + static_cast<int>(i) * (mapWidth + gap);
        DrawText(TextFormat("Floor %d", floorToRender + 1), mapX, startY - 20, 14, {200, 220, 255, 255});
        drawMiniFloor(floorToRender, mapX, startY, miniCell);

        DrawRectangleLines(mapX - 1, startY - 1, mapWidth + 2, mapHeight + 2, Color{170, 185, 200, 255});
    }
}

void RaylibRenderer::drawText(const string& text, int x, int y, int fontSize,
                             unsigned int color) {
    Color c = GetColor(color);
    DrawText(text.c_str(), x, y, fontSize, c);
}

void RaylibRenderer::drawCenteredText(const string& text, int x, int y, int width, int height, int fontSize, Color color) {
    int textWidth = MeasureText(text.c_str(), fontSize);
    int textX = x + (width - textWidth) / 2;
    int textY = y + (height - fontSize) / 2 - 2;
    DrawText(text.c_str(), textX, textY, fontSize, color);
}

void RaylibRenderer::drawPopup(const string& text) {
    const int popupW = 260;
    const int popupH = 56;
    const int popupX = (windowWidth - popupW) / 2;
    const int popupY = 22;

    DrawRectangle(popupX, popupY, popupW, popupH, Color{30, 35, 45, 240});
    DrawRectangleLinesEx(Rectangle{(float)popupX, (float)popupY, (float)popupW, (float)popupH}, 2.0f, Color{200, 200, 200, 255});
    drawCenteredText(text, popupX, popupY, popupW, popupH, 20, Color{255, 224, 130, 255});
}

void RaylibRenderer::drawGameOverOverlay(const string& title, const string& prompt) {
    DrawRectangle(0, 0, windowWidth, windowHeight, Color{0, 0, 0, 150});

    const int panelW = 640;
    const int panelH = 220;
    const int panelX = (windowWidth - panelW) / 2;
    const int panelY = (windowHeight - panelH) / 2;

    DrawRectangle(panelX, panelY, panelW, panelH, Color{30, 35, 45, 245});
    DrawRectangleLinesEx(Rectangle{(float)panelX, (float)panelY, (float)panelW, (float)panelH}, 3.0f, Color{230, 230, 230, 255});

    drawCenteredText(title, panelX, panelY + 28, panelW, 56, 34, Color{255, 220, 120, 255});
    drawCenteredText(status, panelX, panelY + 94, panelW, 36, 20, WHITE);
    drawCenteredText(prompt, panelX, panelY + 145, panelW, 42, 22, Color{120, 220, 255, 255});
}
