#include "DebugWindow.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>

namespace {
const Color kWhite = {255, 255, 255, 255};
const Color kBlack = {0, 0, 0, 255};
const Color kGrey = {160, 160, 160, 255};
const Color kBorder = {180, 180, 180, 255};
const Color kSoftHighlight = {245, 245, 245, 255};
const Color kHeader = {250, 250, 250, 255};
const Color kActive = {255, 239, 186, 255};
const Color kCompleted = {226, 244, 226, 255};
const Color kCancelled = {248, 215, 218, 255};
}

DebugWindow::DebugWindow(const string& snapshotFilePath)
        : snapshotPath(snapshotFilePath), width(1260), height(900), panelPadding(18), running(false),
            titleFontSize(26), headerFontSize(21), contentFontSize(17), goalScroll(0.0f) {}

DebugWindow::~DebugWindow() { closeWindow(); }

void DebugWindow::initWindow() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, "Debugging / Logging Window");
    SetWindowMinSize(1100, 760);
    SetTargetFPS(30);
    // Prefer a cleaner system font on macOS; fall back to the default font.
    font = LoadFontEx("/System/Library/Fonts/Supplemental/Arial.ttf", 18, nullptr, 0);
    if (font.texture.id == 0) {
        font = LoadFontEx("/System/Library/Fonts/Menlo.ttc", 18, nullptr, 0);
    }
    if (font.texture.id == 0) {
        font = GetFontDefault();
    }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    running = true;
}

void DebugWindow::closeWindow() {
    if (running && IsWindowReady()) {
        CloseWindow();
    }
    running = false;
}

bool DebugWindow::isWindowOpen() const { return running && !WindowShouldClose(); }

void DebugWindow::run() {
    while (isWindowOpen()) {
        updateSnapshot();
        render();
    }
}

void DebugWindow::updateSnapshot() { loadSnapshotFromFile(); }

namespace {
vector<string> extractObjectsFromBlock(const string& block) {
    vector<string> objects;
    size_t cursor = 0;
    while (cursor < block.size()) {
        size_t open = block.find('{', cursor);
        if (open == string::npos) break;
        int depth = 0;
        size_t close = open;
        for (; close < block.size(); ++close) {
            if (block[close] == '{') {
                ++depth;
            } else if (block[close] == '}') {
                --depth;
                if (depth == 0) {
                    objects.push_back(block.substr(open, close - open + 1));
                    cursor = close + 1;
                    break;
                }
            }
        }
        if (close >= block.size()) {
            break;
        }
    }
    return objects;
}

string extractArrayBlock(const string& content, const string& key) {
    size_t keyPos = content.find(key);
    if (keyPos == string::npos) return "";
    size_t open = content.find('[', keyPos + key.size());
    if (open == string::npos) return "";
    int depth = 0;
    for (size_t i = open; i < content.size(); ++i) {
        if (content[i] == '[') {
            ++depth;
        } else if (content[i] == ']') {
            --depth;
            if (depth == 0) {
                return content.substr(open + 1, i - open - 1);
            }
        }
    }
    return "";
}

string extractBracketValue(const string& content, const string& key) {
    size_t keyPos = content.find(key);
    if (keyPos == string::npos) return "";
    size_t open = content.find('[', keyPos + key.size());
    if (open == string::npos) return "";
    int depth = 0;
    for (size_t i = open; i < content.size(); ++i) {
        if (content[i] == '[') {
            ++depth;
        } else if (content[i] == ']') {
            --depth;
            if (depth == 0) {
                return content.substr(open, i - open + 1);
            }
        }
    }
    return "";
}

string extractObjectBlock(const string& content, const string& key) {
    size_t keyPos = content.find(key);
    if (keyPos == string::npos) return "";
    size_t open = content.find('{', keyPos + key.size());
    if (open == string::npos) return "";
    int depth = 0;
    for (size_t i = open; i < content.size(); ++i) {
        if (content[i] == '{') {
            ++depth;
        } else if (content[i] == '}') {
            --depth;
            if (depth == 0) {
                return content.substr(open, i - open + 1);
            }
        }
    }
    return "";
}
} // namespace

string DebugWindow::trim(const string& value) const {
    const string whitespace = " \t\n\r";
    size_t start = value.find_first_not_of(whitespace);
    if (start == string::npos) return "";
    size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

vector<string> DebugWindow::splitLines(const string& text) const {
    vector<string> lines;
    string token;
    stringstream ss(text);
    while (getline(ss, token, '\n')) {
        lines.push_back(token);
    }
    return lines;
}

string DebugWindow::joinLines(const vector<string>& lines) const {
    stringstream ss;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) ss << '\n';
        ss << lines[i];
    }
    return ss.str();
}

string DebugWindow::extractStringValue(const string& objectText, const string& key) const {
    size_t pos = objectText.find(key);
    if (pos == string::npos) return "";
    pos = objectText.find('"', pos + key.size());
    if (pos == string::npos) return "";
    size_t end = objectText.find('"', pos + 1);
    if (end == string::npos) return "";
    return objectText.substr(pos + 1, end - pos - 1);
}

bool DebugWindow::extractBoolValue(const string& objectText, const string& key) const {
    size_t pos = objectText.find(key);
    if (pos == string::npos) return false;
    size_t valuePos = objectText.find_first_not_of(" :", pos + key.size());
    if (valuePos == string::npos) return false;
    return objectText.compare(valuePos, 4, "true") == 0;
}

int DebugWindow::extractIntValue(const string& objectText, const string& key) const {
    size_t pos = objectText.find(key);
    if (pos == string::npos) return 0;
    size_t valuePos = objectText.find_first_of("-0123456789", pos + key.size());
    if (valuePos == string::npos) return 0;
    size_t end = objectText.find_first_not_of("0123456789-", valuePos);
    return stoi(objectText.substr(valuePos, end - valuePos));
}

float DebugWindow::extractFloatValue(const string& objectText, const string& key) const {
    size_t pos = objectText.find(key);
    if (pos == string::npos) return 0.0f;
    size_t valuePos = objectText.find_first_of("-0123456789", pos + key.size());
    if (valuePos == string::npos) return 0.0f;
    size_t end = objectText.find_first_not_of("0123456789.-", valuePos);
    return stof(objectText.substr(valuePos, end - valuePos));
}

vector<string> DebugWindow::extractStringArray(const string& objectText, const string& key) const {
    vector<string> values;
    size_t pos = objectText.find(key);
    if (pos == string::npos) return values;
    size_t open = objectText.find('[', pos + key.size());
    size_t close = objectText.find(']', open + 1);
    if (open == string::npos || close == string::npos || close <= open + 1) return values;
    string content = objectText.substr(open + 1, close - open - 1);
    size_t cursor = 0;
    while (cursor < content.size()) {
        size_t q1 = content.find('"', cursor);
        if (q1 == string::npos) break;
        size_t q2 = content.find('"', q1 + 1);
        if (q2 == string::npos) break;
        values.push_back(content.substr(q1 + 1, q2 - q1 - 1));
        cursor = q2 + 1;
    }
    return values;
}

DebugGoalLine DebugWindow::parseGoalObject(const vector<string>& lines) const {
    string text = joinLines(lines);
    DebugGoalLine goal;
    goal.expression = extractStringValue(text, "\"expression\"");
    goal.operatorName = extractStringValue(text, "\"operatorName\"");
    goal.status = extractStringValue(text, "\"status\"");
    goal.isOperator = extractBoolValue(text, "\"isOperator\"");
    goal.preconditions = extractStringArray(text, "\"preconditions\"");
    goal.effects = extractStringArray(text, "\"effects\"");
    return goal;
}

DebugSuccessorLine DebugWindow::parseSuccessorObject(const vector<string>& lines) const {
    string text = joinLines(lines);
    DebugSuccessorLine successor;
    successor.label = extractStringValue(text, "\"label\"");
    if (successor.label.empty()) {
        successor.label = extractStringValue(text, "\"pos\"");
    }
    successor.g = extractIntValue(text, "\"g\"");
    successor.h = extractIntValue(text, "\"h\"");
    successor.f = extractIntValue(text, "\"f\"");
    successor.chosen = extractBoolValue(text, "\"chosen\"");
    return successor;
}

bool DebugWindow::loadSnapshotFromFile() {
    ifstream file(snapshotPath);
    if (!file.is_open()) return false;

    DebugSnapshot loaded;
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    loaded.agent = extractStringValue(content, "\"agent\"");
    loaded.state = extractStringValue(content, "\"state\"");
    loaded.stepsTaken = extractIntValue(content, "\"stepsTaken\"");
    loaded.currentCell = extractBracketValue(content, "\"currentCell\"");
    loaded.targetCell = extractBracketValue(content, "\"targetCell\"");
    loaded.activeGoalType = extractStringValue(content, "\"activeGoalType\"");

    auto parseSection = [&](const string& key, auto&& appendFn) {
        string block = extractArrayBlock(content, key);
        for (const auto& objectText : extractObjectsFromBlock(block)) {
            appendFn(objectText);
        }
    };

    parseSection("\"goalStack\"", [&](const string& objectText) {
        loaded.goalStack.push_back(parseGoalObject({objectText}));
    });
    parseSection("\"completedGoals\"", [&](const string& objectText) {
        loaded.completedGoals.push_back(parseGoalObject({objectText}));
    });
    parseSection("\"cancelledGoals\"", [&](const string& objectText) {
        loaded.cancelledGoals.push_back(parseGoalObject({objectText}));
    });
    parseSection("\"astarSuccessors\"", [&](const string& objectText) {
        loaded.successors.push_back(parseSuccessorObject({objectText}));
    });

    string chosenObject = extractObjectBlock(content, "\"chosenNode\"");
    if (!chosenObject.empty()) {
        loaded.chosenNode = parseSuccessorObject({chosenObject});
        loaded.hasChosenNode = true;
    }

    bool hasContent = !loaded.agent.empty() || !loaded.state.empty() || !loaded.goalStack.empty() ||
                      !loaded.completedGoals.empty() || !loaded.cancelledGoals.empty() ||
                      !loaded.successors.empty() || loaded.hasChosenNode;
    if (hasContent) {
        snapshot = loaded;
        return true;
    }
    return false;
}

void DebugWindow::render() {
    BeginDrawing();
    ClearBackground(kWhite);
    renderHeader();
    // if snapshot empty, show waiting message
    bool hasContent = !snapshot.agent.empty() || !snapshot.state.empty() || !snapshot.goalStack.empty() ||
                      !snapshot.completedGoals.empty() || !snapshot.cancelledGoals.empty() || !snapshot.successors.empty() || snapshot.hasChosenNode;
    if (!hasContent) {
        Vector2 txt = MeasureTextEx(font, "Waiting for snapshot...", titleFontSize, 0);
        DrawTextEx(font, "Waiting for snapshot...", {(float)(width/2) - txt.x/2, (float)(height/2) - txt.y/2}, titleFontSize, 0, kGrey);
    } else {
        renderPanels();
    }
    EndDrawing();
}

void DebugWindow::renderHeader() {
    DrawRectangle(0, 0, width, 72, kHeader);
    DrawLine(8, 72, width - 8, 72, kBorder);
    // Title centered
    Vector2 titleSize = MeasureTextEx(font, "Debugging / Logging Window", titleFontSize, 0);
    DrawTextEx(font, "Debugging / Logging Window", {(float)(width / 2) - titleSize.x / 2, 18}, titleFontSize, 0, kBlack);
}

void DebugWindow::renderPanels() {
    int contentTop = 84;
    int panelHeight = height - contentTop - 16;
    // Two-column layout: Goal Stack (left) and A* Successors (right)
    int panelWidth = (width - 3 * panelPadding) / 2;
    int x1 = panelPadding;
    int x2 = x1 + panelWidth + panelPadding;

    DrawRectangleLines(x1, contentTop, panelWidth, panelHeight, kBorder);
    DrawRectangleLines(x2, contentTop, panelWidth, panelHeight, kBorder);

    renderGoalStackPanel(x1 + panelPadding, contentTop + panelPadding, panelWidth - 2 * panelPadding, panelHeight - 2 * panelPadding);
    renderAStarPanel(x2 + panelPadding, contentTop + panelPadding, panelWidth - 2 * panelPadding, panelHeight - 2 * panelPadding);
}

void DebugWindow::renderGoalCard(const DebugGoalLine& line, int x, int& y, int width, int height) {
    Color textColor = kWhite;
    const bool isPreconditionLine =
        line.expression.find("sameFloor(") != string::npos ||
        line.expression.find("pathAvailable(") != string::npos ||
        line.expression.find("onTransitionCell(") != string::npos ||
        line.expression.find('^') != string::npos;

    if (line.status == "COMPLETED") textColor = {0, 120, 0, 255};
    else if (line.status == "CANCELLED") textColor = {180, 30, 30, 255};
    else if (line.status == "PERFORMING") textColor = {0, 130, 0, 255};
    else if (line.status == "ACTIVE") textColor = {200, 160, 0, 255};
    else if (isPreconditionLine) textColor = {120, 0, 120, 255};
    else textColor = kBlack;

    string label = string("[") + (line.status.empty() ? "PENDING" : line.status) + "]  ";
    label += line.expression.empty() ? "(no goals)" : line.expression;

    // Draw text only (no boxed empty boxes). Completed show strike-through.
    DrawTextEx(font, label.c_str(), {(float)x, (float)y}, contentFontSize, 0, textColor);
    if (line.status == "COMPLETED") {
        int w = MeasureTextEx(font, label.c_str(), contentFontSize, 0).x;
        DrawLine(x, y + contentFontSize / 2, x + w, y + contentFontSize / 2, textColor);
    }
    y += contentFontSize + 8;
}

void DebugWindow::renderGoalStackPanel(int x, int y, int width, int height) {
    DrawTextEx(font, "GOAL STACK", {(float)x, (float)y}, headerFontSize, 0, kBlack);
    y += headerFontSize + 8;
    if (!snapshot.agent.empty()) {
        DrawTextEx(font, (string("Agent: ") + snapshot.agent).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 4;
    }
    if (snapshot.stepsTaken > 0 || !snapshot.agent.empty()) {
        DrawTextEx(font, (string("Steps Taken: ") + to_string(snapshot.stepsTaken)).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 4;
    }
    if (!snapshot.activeGoalType.empty()) {
        DrawTextEx(font, (string("GoalType: ") + snapshot.activeGoalType).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 8;
    }

    DrawLine(x, y, x + width, y, kBorder);
    y += 8;

    // scrolling: mouse wheel
    float wheel = GetMouseWheelMove();
    goalScroll -= wheel * 20.0f;
    if (goalScroll < 0) goalScroll = 0;

    int contentY = y;
    float cursorY = (float)contentY - goalScroll;

    if (snapshot.goalStack.empty()) {
        DrawTextEx(font, "(no goals)", {(float)x, cursorY}, contentFontSize, 0, kGrey);
        cursorY += contentFontSize + 8;
    } else {
        // show top-first: snapshot.goalStack stores top at index 0 per writer
            // Determine main (top-level) goal: last non-operator in the list
            int mainGoalIndex = -1;
            for (int i = (int)snapshot.goalStack.size() - 1; i >= 0; --i) {
                if (!snapshot.goalStack[i].isOperator) {
                    mainGoalIndex = i;
                    break;
                }
            }
            for (size_t idx = 0; idx < snapshot.goalStack.size(); ++idx) {
                const auto& line = snapshot.goalStack[idx];
                Color textColor = kBlack;
                // Main goal (lowest non-operator in stack) -> RED
                if ((int)idx == mainGoalIndex) {
                    textColor = {180, 40, 40, 255}; // red
                }
                // Precondition subgoals -> purple
                else if (line.expression.find("sameFloor(") != string::npos ||
                         line.expression.find("pathAvailable(") != string::npos ||
                         line.expression.find("onTransitionCell(") != string::npos ||
                         line.expression.find("^") != string::npos) {
                    textColor = {120, 0, 120, 255}; // purple
                }
                // ACTIVE -> yellow
                else if (line.status == "ACTIVE") textColor = {200,160,0,255};
                // PERFORMING -> green (operator currently executing)
                else if (line.status == "PERFORMING") textColor = {0,150,0,255};
                else if (line.status == "PENDING") textColor = kBlack;
                else if (line.status == "COMPLETED") textColor = kGrey;
                else if (line.status == "CANCELLED") textColor = {180,30,30,255};

                string label = string("[") + (line.status.empty() ? "PENDING" : line.status) + "]  ";
                label += line.expression.empty() ? "(no goals)" : line.expression;
                DrawTextEx(font, label.c_str(), {(float)x, cursorY}, contentFontSize, 0, textColor);
                if (line.status == "COMPLETED") {
                    int textWidth = MeasureTextEx(font, label.c_str(), contentFontSize, 0).x;
                    DrawLine(x, static_cast<int>(cursorY) + contentFontSize / 2,
                             x + textWidth, static_cast<int>(cursorY) + contentFontSize / 2,
                             textColor);
                }
                cursorY += contentFontSize + 8;
        }
    }

    // Draw completed and cancelled sections
    float afterY = contentY + 200; // small placeholder if empty
    if (!snapshot.completedGoals.empty()) {
        DrawTextEx(font, "COMPLETED", {(float)x, afterY}, headerFontSize - 4, 0, kBlack);
        afterY += headerFontSize;
        for (const auto& line : snapshot.completedGoals) {
            DrawTextEx(font, (string("~~") + line.expression + "~~").c_str(), {(float)x, afterY}, contentFontSize, 0, kGrey);
            afterY += contentFontSize + 4;
        }
    }

    if (!snapshot.cancelledGoals.empty()) {
        DrawTextEx(font, "CANCELLED", {(float)x, afterY + 8}, headerFontSize - 4, 0, kBlack);
        afterY += headerFontSize + 8;
        for (const auto& line : snapshot.cancelledGoals) {
            DrawTextEx(font, line.expression.c_str(), {(float)x, afterY}, contentFontSize, 0, kBlack);
            afterY += contentFontSize + 4;
        }
    }
}

void DebugWindow::renderOperatorPanel(int x, int y, int width, int height) {
    DrawTextEx(font, "ACTIVE OPERATOR", {(float)x, (float)y}, headerFontSize, 0, kBlack);
    y += headerFontSize + 8;
    if (!snapshot.currentCell.empty()) {
        DrawTextEx(font, (string("Current Cell: ") + snapshot.currentCell).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 4;
    }
    if (!snapshot.targetCell.empty()) {
        DrawTextEx(font, (string("Target Cell: ") + snapshot.targetCell).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 8;
    }

    const DebugGoalLine* activeOperator = nullptr;
    if (!snapshot.goalStack.empty() && snapshot.goalStack.front().isOperator) {
        activeOperator = &snapshot.goalStack.front();
    }

    if (!activeOperator) {
        DrawTextEx(font, "No active operator", {(float)x, (float)y}, contentFontSize, 0, kGrey);
        return;
    }

    string operatorLabel = activeOperator->operatorName.empty() ? activeOperator->expression : activeOperator->operatorName;
    DrawTextEx(font, (string("Operator: ") + operatorLabel).c_str(), {(float)x, (float)y}, headerFontSize - 2, 0, kBlack);
    y += headerFontSize;
    DrawLine(x, y, x + width, y, kBorder);
    y += 8;

    DrawTextEx(font, "Preconditions:", {(float)x, (float)y}, contentFontSize, 0, kBlack);
    y += contentFontSize + 6;
    for (const auto& pre : activeOperator->preconditions) {
        DrawTextEx(font, (string(u8"  ✓  ") + pre).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 4;
    }
    y += 6;
    DrawTextEx(font, "Effects:", {(float)x, (float)y}, contentFontSize, 0, kBlack);
    y += contentFontSize + 6;
    for (const auto& eff : activeOperator->effects) {
        DrawTextEx(font, (string(u8"  →  ") + eff).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
        y += contentFontSize + 4;
    }
}

void DebugWindow::renderAStarPanel(int x, int y, int width, int height) {
    DrawTextEx(font, "A* SUCCESSORS", {(float)x, (float)y}, headerFontSize, 0, kBlack);
    y += headerFontSize + 8;

    if (snapshot.currentCell.empty() && snapshot.targetCell.empty() && snapshot.successors.empty()) {
        DrawTextEx(font, "No search data", {(float)x, (float)y + 24}, contentFontSize, 0, kGrey);
        return;
    }

    DrawTextEx(font, (string("Current Cell: ") + snapshot.currentCell).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
    y += contentFontSize + 4;
    DrawTextEx(font, (string("Target Cell:  ") + snapshot.targetCell).c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
    y += contentFontSize + 8;

    // Table header
    int tableLeft = x - 2;
    int tableTop = y - 2;
    int rowHeight = contentFontSize + 8;
    int tableWidth = width - 8;
    int headerHeight = contentFontSize + 6;
    int columnPos = x;
    int columnG = x + 110;
    int columnH = x + 170;
    int columnF = x + 220;

    DrawRectangleLines(tableLeft, tableTop, tableWidth, headerHeight, kBorder);
    DrawTextEx(font, "Pos", {(float)x, (float)y}, contentFontSize, 0, kBlack);
    DrawTextEx(font, "g", {(float)columnG, (float)y}, contentFontSize, 0, kBlack);
    DrawTextEx(font, "h", {(float)columnH, (float)y}, contentFontSize, 0, kBlack);
    DrawTextEx(font, "f", {(float)columnF, (float)y}, contentFontSize, 0, kBlack);
    y += contentFontSize + 6;

    for (const auto& succ : snapshot.successors) {
        Color textCol = succ.chosen ? (Color){200,160,0,255} : kBlack;
        if (succ.chosen) {
            DrawRectangle(x - 4, y - 2, width - 8, contentFontSize + 8, kActive);
        }
        DrawRectangleLines(tableLeft, y - 2, tableWidth, rowHeight, kBorder);
        DrawTextEx(font, succ.label.c_str(), {(float)x, (float)y}, contentFontSize, 0, textCol);
        string gText = to_string(succ.g);
        string hText = to_string(succ.h);
        string fText = to_string(succ.f);
        DrawTextEx(font, gText.c_str(), {(float)columnG, (float)y}, contentFontSize, 0, textCol);
        DrawTextEx(font, hText.c_str(), {(float)columnH, (float)y}, contentFontSize, 0, textCol);
        DrawTextEx(font, fText.c_str(), {(float)columnF, (float)y}, contentFontSize, 0, textCol);
        y += rowHeight;
    }

    if (snapshot.hasChosenNode) {
        y += 8;
        DrawTextEx(font, "Chosen Node", {(float)x, (float)y}, headerFontSize - 4, 0, kBlack);
        y += headerFontSize;
        string chosenLine = string("Pos: ") + snapshot.chosenNode.label + string("   g=") + to_string(snapshot.chosenNode.g) + string("   h=") + to_string(snapshot.chosenNode.h) + string("   f=") + to_string(snapshot.chosenNode.f);
        DrawTextEx(font, chosenLine.c_str(), {(float)x, (float)y}, contentFontSize, 0, kBlack);
    }
}

void DebugWindow::drawStrikeThroughText(const string& text, int x, int y, int font, Color color) {
    DrawText(text.c_str(), x, y, font, color);
    int width = MeasureText(text.c_str(), font);
    DrawLine(x, y + font / 2, x + width, y + font / 2, color);
}
