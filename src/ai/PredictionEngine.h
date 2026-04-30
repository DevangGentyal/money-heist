#ifndef PREDICTION_ENGINE_H
#define PREDICTION_ENGINE_H

#include "../grid/Grid3D.h"
#include <vector>

using namespace std;

struct PathPrediction {
    vector<Position> predictedPath;
    Position interceptionPoint;
    int pathDepth;
    float confidence;
};

class PredictionEngine {
private:
    const Grid3D& grid;
    int predictionDepth;
    
public:
    PredictionEngine(const Grid3D& g, int depth = 10);
    
    // Predict where target is heading
    Position predictTargetGoal(const Position& targetPos, 
                             const Position& vault,
                             const Position& exit);
    
    // Simulate path to predicted goal
    vector<Position> simulatePath(const Position& from, 
                                 const Position& goal,
                                 int maxSteps = 15);
    
    // Find best interception point
    Position getInterceptionPoint(const Position& targetPos,
                                 const Position& targetGoal,
                                 int depth);
    
    // Calculate interception confidence
    float calculateConfidence(const Position& currentPos,
                            const vector<Position>& predictedPath);
    
    void setPredictionDepth(int d) { predictionDepth = d; }
};

#endif
