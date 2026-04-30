#include "PredictionEngine.h"
#include "AStar3D.h"
#include <cmath>
#include <algorithm>

PredictionEngine::PredictionEngine(const Grid3D& g, int depth) 
    : grid(g), predictionDepth(depth) {}

Position PredictionEngine::predictTargetGoal(const Position& targetPos, 
                                            const Position& vault,
                                            const Position& exit) {
    // If closer to exit after vault, likely going to exit
    int distToVault = grid.manhattanDistance(targetPos, vault);
    int distToExit = grid.manhattanDistance(targetPos, exit);
    
    // Robber typically goes vault -> exit
    if (distToVault < distToExit) {
        return vault; // First intercept at vault
    } else {
        return exit; // Then intercept at exit
    }
}

vector<Position> PredictionEngine::simulatePath(const Position& from, 
                                               const Position& goal,
                                               int maxSteps) {
    // Use A* to simulate the path the target would take
    vector<Position> path = AStar3D::findPath(from, goal, grid);
    
    if ((int)path.size() > maxSteps) {
        path.resize(maxSteps);
    }
    
    return path;
}

Position PredictionEngine::getInterceptionPoint(const Position& targetPos,
                                               const Position& targetGoal,
                                               int depth) {
    // Simulate target's path
    vector<Position> predictedPath = simulatePath(targetPos, targetGoal, depth);
    
    if (predictedPath.empty()) {
        return targetPos;
    }
    
    // Choose point at depth, or last point if path is shorter
    if ((int)predictedPath.size() > depth) {
        return predictedPath[depth];
    }
    
    return predictedPath.back();
}

float PredictionEngine::calculateConfidence(const Position& currentPos,
                                          const vector<Position>& predictedPath) {
    if (predictedPath.empty()) return 0.0f;
    
    // Confidence based on distance to predicted path
    float minDist = 1000000.0f;
    for (const auto& p : predictedPath) {
        float dist = grid.manhattanDistance(currentPos, p);
        minDist = min(minDist, dist);
    }
    
    // Closer = higher confidence
    if (minDist <= 2) return 0.9f;
    if (minDist <= 4) return 0.7f;
    if (minDist <= 6) return 0.5f;
    return 0.2f;
}
