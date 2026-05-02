#ifndef DEBUG_WINDOW_H
#define DEBUG_WINDOW_H

#include "raylib.h"
#include <string>
#include <vector>

using namespace std;

struct DebugGoalLine {
    string expression;
    string operatorName;
    string status;
    bool isOperator;
    vector<string> preconditions;
    vector<string> effects;
};

struct DebugSuccessorLine {
    string label;
    int g;
    int h;
    int f;
    bool chosen;
};

struct DebugSnapshot {
    string agent;
    string state;
    string currentCell;
    string targetCell;
    string activeGoalType;
    vector<DebugGoalLine> goalStack;
    vector<DebugGoalLine> completedGoals;
    vector<DebugGoalLine> cancelledGoals;
    vector<DebugSuccessorLine> successors;
    DebugSuccessorLine chosenNode;
    bool hasChosenNode;
    string reasoning;

    DebugSnapshot() : hasChosenNode(false) {}
};

class DebugWindow {
private:
    string snapshotPath;
    int width;
    int height;
    int panelPadding;
    bool running;
    DebugSnapshot snapshot;
    // UI state
    Font font;
    int titleFontSize;
    int headerFontSize;
    int contentFontSize;
    float goalScroll;

public:
    explicit DebugWindow(const string& snapshotFilePath);
    ~DebugWindow();

    void initWindow();
    void run();
    void closeWindow();
    bool isWindowOpen() const;

private:
    void updateSnapshot();
    bool loadSnapshotFromFile();
    void render();
    void renderHeader();
    void renderPanels();
    void renderGoalStackPanel(int x, int y, int width, int height);
    void renderOperatorPanel(int x, int y, int width, int height);
    void renderAStarPanel(int x, int y, int width, int height);
    void renderGoalCard(const DebugGoalLine& line, int x, int& y, int width, int height);
    void drawStrikeThroughText(const string& text, int x, int y, int font, Color color);
    vector<string> splitLines(const string& text) const;
    string trim(const string& value) const;
    vector<string> extractStringArray(const string& objectText, const string& key) const;
    string extractStringValue(const string& objectText, const string& key) const;
    bool extractBoolValue(const string& objectText, const string& key) const;
    int extractIntValue(const string& objectText, const string& key) const;
    float extractFloatValue(const string& objectText, const string& key) const;
    string joinLines(const vector<string>& lines) const;
    DebugGoalLine parseGoalObject(const vector<string>& lines) const;
    DebugSuccessorLine parseSuccessorObject(const vector<string>& lines) const;
};

#endif
