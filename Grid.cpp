#include "Grid.h"

using namespace std;

Grid::Grid() {
    loadDefaultMap();
}

void Grid::loadDefaultMap() {
    vector<string> defaultMap = {
        "###########",
        "#R...#...E#",
        "#.#.#.#.#.#",
        "#...V.....#",
        "#.###.#.#.#",
        "#.....#...#",
        "#.#.#.#.#.#",
        "#...#.....#",
        "#.#.#.###.#",
        "#...#...P.#",
        "###########"
    };

    height = defaultMap.size();
    width = defaultMap[0].size();
    map.resize(height, vector<char>(width));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            char cell = defaultMap[y][x];
            if (cell == 'R') { initialRobberPos = {x, y}; map[y][x] = '.'; }
            else if (cell == 'P') { initialPolicePos = {x, y}; map[y][x] = '.'; }
            else { map[y][x] = cell; }
            if (cell == 'V') vaultPos = {x, y};
            else if (cell == 'E') exitPos = {x, y};
        }
    }
}

bool Grid::isWall(int x, int y) const {
    if (!isValid(x, y)) return true;
    return map[y][x] == '#';
}

bool Grid::isValid(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

char Grid::getCell(int x, int y) const {
    if (!isValid(x, y)) return '#';
    return map[y][x];
}

int Grid::getWidth() const { return width; }
int Grid::getHeight() const { return height; }
