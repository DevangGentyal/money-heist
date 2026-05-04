#include "Grid3D.h"
#include <algorithm>
#include <cmath>

Grid3D::Grid3D() : width(15), height(15), depth(3)
{
  map.resize(depth, vector<vector<CellType>>(
                        height, vector<CellType>(width, CellType::EMPTY)));
}

void Grid3D::loadLevel(int difficulty)
{
  depth = difficulty;
  if (depth < 1)
    depth = 1;
  if (depth > 3)
    depth = 3;
  map.assign(depth, vector<vector<CellType>>(
                        height, vector<CellType>(width, CellType::EMPTY)));

  for (int z = 0; z < depth; z++)
  {
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        map[z][y][x] = CellType::EMPTY;
      }
    }
  }
  initialPolicePos.clear();
  cctvPositions.clear();
  alertZones.clear();

  // Symbol legend for floor strings:
  // # wall, . empty, R robber start, P police start, V vault, X exit,
  // U upstairs, D downstairs, A alert, C cctv
  auto applyFloor = [&](int z, const vector<string> &rows)
  {
    if (z < 0 || z >= depth)
      return;
    if (static_cast<int>(rows.size()) != height)
      return;

    for (int y = 0; y < height; ++y)
    {
      if (static_cast<int>(rows[y].size()) != width)
        continue;
      for (int x = 0; x < width; ++x)
      {
        char tile = rows[y][x];
        Position p(x, y, z);

        switch (tile)
        {
        case '#':
          map[z][y][x] = CellType::WALL;
          break;
        case 'R':
          initialRobberPos = p;
          map[z][y][x] = CellType::EMPTY;
          break;
        case 'P':
          initialPolicePos.push_back(p);
          map[z][y][x] = CellType::EMPTY;
          break;
        case 'V':
          vaultPos = p;
          map[z][y][x] = CellType::VAULT;
          break;
        case 'X':
          exitPos = p;
          map[z][y][x] = CellType::EXIT;
          break;
        case 'U':
          map[z][y][x] = CellType::STAIRS;
          break;
        case 'D':
          map[z][y][x] = CellType::ELEVATOR;
          break;
        case 'A':
          map[z][y][x] = CellType::ALERT_ZONE;
          alertZones.push_back(p);
          break;
        case 'C':
          map[z][y][x] = CellType::CCTV_ZONE;
          cctvPositions.push_back(p);
          break;
        default:
          map[z][y][x] = CellType::EMPTY;
          break;
        }
      }
    }
  };

  const vector<string> easyFloor0 = {
      "###############", 
      "#.............#", 
      "#.X.......V.A.#",
      "#...#.....#...#", 
      "#...#.....#...#", 
      "#.........#...#",
      "#.###.###.....#", 
      "#.............#", 
      "#...#.....#...#",
      "#...#.....#..P#", 
      "#...#.....#...#", 
      "#..###.###....#",
      "#.A.........R.#", 
      "#.............#", 
      "###############"};

  const vector<string> normalFloor0 = {
      "###############", "#.............#", "#.X...U.....A.#",
      "#...#.....#...#", "#...#.....#...#", "#.........#...#",
      "#.###.###.....#", "#.............#", "#...#.....#...#",
      "#...#.....#..P#", "#...#.....#...#", "#..###.###....#",
      "#.A.........R.#", "#.............#", "###############"};

  const vector<string> normalFloor1 = {
      "###############", "#.............#", "#.....D.....A.#",
      "#...#.....#...#", "#...#.....#...#", "#.........#...#",
      "#.###.###.....#", "#.............#", "#...#..P..#...#",
      "#...#.....#...#", "#...#.....V...#", "#..###.###....#",
      "#.A...........#", "#.............#", "###############"};

  const vector<string> hardFloor0 = {
      "###############", "#...........E.#", "#.A.U.........#",
      "#...#.....#...#", "#...#.....#...#", "#.........#...#",
      "#.###.###.....#", "#.............#", "#...#..X..#...#",
      "#...#.....#..P#", "#...#.....#...#", "#..###.###....#",
      "#.A.........R.#", "#.............#", "###############"};

  const vector<string> hardFloor1 = {
      "###############", "#.............#", "#.A.D.........#",
      "#...#.....#...#", "#...#.....#...#", "#.........#...#",
      "#.###.###.....#", "#.............#", "#...#..P..#...#",
      "#...#.....#.U.#", "#...#.....#...#", "#..###.###....#",
      "#.A...........#", "#.............#", "###############"};

  const vector<string> hardFloor2 = {
      "###############", "#.............#", "#...........A.#",
      "#...#.....#...#", "#...#.....#...#", "#.........#...#",
      "#.###.###.....#", "#.............#", "#...#.....#...#",
      "#...#.....#.D.#", "#...#.....V...#", "#..###.###....#",
      "#.A.....P.....#", "#.............#", "###############"};

  if (difficulty == 1)
  {
    applyFloor(0, easyFloor0);
  }
  else if (difficulty == 2)
  {
    applyFloor(0, normalFloor0);
    applyFloor(1, normalFloor1);
  }
  else
  {
    applyFloor(0, hardFloor0);
    applyFloor(1, hardFloor1);
    applyFloor(2, hardFloor2);
  }
}

bool Grid3D::isValid(const Position &p) const
{
  return p.x > 0 && p.x < width - 1 && p.y > 0 && p.y < height - 1 &&
         p.z >= 0 && p.z < depth;
}

bool Grid3D::isWall(const Position &p) const
{
  if (!isValid(p))
    return true;
  return map[p.z][p.y][p.x] == CellType::WALL;
}

bool Grid3D::isCCTVZone(const Position &p) const
{
  if (!isValid(p))
    return false;
  return map[p.z][p.y][p.x] == CellType::CCTV_ZONE;
}

bool Grid3D::isAlertZone(const Position &p) const
{
  if (!isValid(p))
    return false;
  return map[p.z][p.y][p.x] == CellType::ALERT_ZONE;
}

CellType Grid3D::getCell(const Position &p) const
{
  if (!isValid(p))
    return CellType::WALL;
  return map[p.z][p.y][p.x];
}

void Grid3D::setCell(const Position &p, CellType type)
{
  if (isValid(p))
  {
    map[p.z][p.y][p.x] = type;
  }
}

int Grid3D::manhattanDistance(const Position &p1, const Position &p2) const
{
  return abs(p1.x - p2.x) + abs(p1.y - p2.y) + abs(p1.z - p2.z);
}

int Grid3D::euclideanDistance(const Position &p1, const Position &p2) const
{
  int dx = p1.x - p2.x;
  int dy = p1.y - p2.y;
  int dz = p1.z - p2.z;
  return sqrt(dx * dx + dy * dy + dz * dz);
}

int Grid3D::verticalCost(const Position &p1, const Position &p2) const
{
  return abs(p1.z - p2.z) * 3; // Vertical movement costs 3x normal
}

vector<Position> Grid3D::getNeighbors(const Position &p,
                                      bool allowFloorTransitions) const
{
  vector<Position> neighbors;

  // Horizontal movement is always 4-directional.
  int dx[] = {-1, 1, 0, 0};
  int dy[] = {0, 0, -1, 1};

  for (int i = 0; i < 4; i++)
  {
    Position neighbor(p.x + dx[i], p.y + dy[i], p.z);
    if (isValid(neighbor) && !isWall(neighbor))
    {
      neighbors.push_back(neighbor);
    }
  }

  // Vertical movement is allowed only on directional stair cells (and only if
  // transitions are allowed).
  if (allowFloorTransitions)
  {
    CellType currentType = getCell(p);
    if (currentType == CellType::STAIRS)
    {
      Position up(p.x, p.y, p.z + 1);
      if (isValid(up) && !isWall(up))
      {
        neighbors.push_back(up);
      }
    }

    if (currentType == CellType::ELEVATOR)
    {
      Position down(p.x, p.y, p.z - 1);
      if (isValid(down) && !isWall(down))
      {
        neighbors.push_back(down);
      }
    }
  }

  return neighbors;
}

int Grid3D::getMovementCost(const Position &from, const Position &to) const
{
  int baseCost = 1;

  // Additional cost for vertical movement (stairs/elevator)
  if (from.z != to.z)
  {
    baseCost = 3;
  }

  // Cost modifier for moving through zones
  if (isCCTVZone(to))
  {
    baseCost += 2;
  }
  if (isAlertZone(to))
  {
    baseCost += 1;
  }

  return baseCost;
}
