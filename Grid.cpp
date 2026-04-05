#include "Grid.h"

Grid::Grid() {
    // Empty construct, initialized by GameEngine loadLevel
}

void Grid::loadLevel(int difficulty) {
    initialPolicePos.clear();
    initialCCTVPos.clear();

    if (difficulty == 1) { // Easy
        depth = 1; height = 11; width = 11;
        map.assign(depth, vector<vector<char>>(height, vector<char>(width, '.')));
        
        vector<string> floor1 = {
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
        for(int y=0; y<height; ++y) {
            for(int x=0; x<width; ++x) {
                char c = floor1[y][x];
                if (c == 'R') { initialRobberPos = {x, y, 0}; map[0][y][x] = '.'; }
                else if (c == 'P') { initialPolicePos.push_back({x, y, 0}); map[0][y][x] = '.'; }
                else if (c == 'C') { initialCCTVPos.push_back({x, y, 0}); map[0][y][x] = '.'; }
                else if (c == 'V') { vaultPos = {x, y, 0}; map[0][y][x] = 'V'; }
                else if (c == 'E') { exitPos = {x, y, 0}; map[0][y][x] = 'E'; }
                else { map[0][y][x] = c; }
            }
        }
    } 
    else if (difficulty == 2) { // Normal
        depth = 2; height = 11; width = 11;
        map.assign(depth, vector<vector<char>>(height, vector<char>(width, '.')));
        
        vector<string> floor1 = {
            "###########",
            "#R...#...U#",
            "#.#.#.#.#.#",
            "#.........#",
            "#.###.#.#.#",
            "#...C.#...#",
            "#.#.#.#.#.#",
            "#...#.....#",
            "#.#.#.###.#",
            "#...#C..P.#",
            "###########"
        };
        vector<string> floor2 = {
            "###########",
            "#V...#...D#",
            "#.#.#.#.#.#",
            "#.........#",
            "#.###.#.#.#",
            "#.....#...#",
            "#.#.#.#.#C#",
            "#...#.....#",
            "#.#.#.###.#",
            "#E..#...P.#",
            "###########"
        };
        
        vector<vector<string>> floors = {floor1, floor2};
        for(int z=0; z<depth; ++z) {
            for(int y=0; y<height; ++y) {
                for(int x=0; x<width; ++x) {
                    char c = floors[z][y][x];
                    if (c == 'R') { initialRobberPos = {x, y, z}; map[z][y][x] = '.'; }
                    else if (c == 'P') { initialPolicePos.push_back({x, y, z}); map[z][y][x] = '.'; }
                    else if (c == 'C') { initialCCTVPos.push_back({x, y, z}); map[z][y][x] = '.'; }
                    else if (c == 'V') { vaultPos = {x, y, z}; map[z][y][x] = 'V'; }
                    else if (c == 'E') { exitPos = {x, y, z}; map[z][y][x] = 'E'; }
                    else { map[z][y][x] = c; }
                }
            }
        }
    }
    else { // Hard
        depth = 3; height = 11; width = 11;
        map.assign(depth, vector<vector<char>>(height, vector<char>(width, '.')));
        vector<string> floor1 = {
            "###########",
            "#R...#...U#",
            "#C#.#.#.#.#",
            "#.........#",
            "#.###.#.#.#",
            "#.....#..P#",
            "#.#.#.#.#.#",
            "#...#...C.#",
            "#.#.#.###.#",
            "#.........#",
            "###########"
        };
        vector<string> floor2 = {
            "###########",
            "#....#...D#",
            "#.#.#.#.#.#",
            "#...P.....#",
            "#.###.#.#U#",
            "#...C.#...#",
            "#.#.#.#.#.#",
            "#...#.....#",
            "#.#.#.###.#",
            "#.........#",
            "###########"
        };
        vector<string> floor3 = {
            "###########",
            "#V...#....#",
            "#.#.#.#.#.#",
            "#C........#",
            "#.###.#.#D#",
            "#.....#...#",
            "#.#.#C#.#.#",
            "#...#...P.#",
            "#.#.#.###.#",
            "#E........#",
            "###########"
        };
        vector<vector<string>> floors = {floor1, floor2, floor3};
        for(int z=0; z<depth; ++z) {
            for(int y=0; y<height; ++y) {
                for(int x=0; x<width; ++x) {
                    char c = floors[z][y][x];
                    if (c == 'R') { initialRobberPos = {x, y, z}; map[z][y][x] = '.'; }
                    else if (c == 'P') { initialPolicePos.push_back({x, y, z}); map[z][y][x] = '.'; }
                    else if (c == 'C') { initialCCTVPos.push_back({x, y, z}); map[z][y][x] = '.'; }
                    else if (c == 'V') { vaultPos = {x, y, z}; map[z][y][x] = 'V'; }
                    else if (c == 'E') { exitPos = {x, y, z}; map[z][y][x] = 'E'; }
                    else { map[z][y][x] = c; }
                }
            }
        }
    }
}

bool Grid::isWall(int x, int y, int z) const {
    if (!isValid(x, y, z)) return true;
    return map[z][y][x] == '#';
}

bool Grid::isValid(int x, int y, int z) const {
    return x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth;
}

char Grid::getCell(int x, int y, int z) const {
    if (!isValid(x, y, z)) return '#';
    return map[z][y][x];
}

void Grid::setCell(int x, int y, int z, char c) {
    if (isValid(x, y, z)) map[z][y][x] = c;
}
