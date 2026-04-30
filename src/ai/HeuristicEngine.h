#ifndef HEURISTIC_ENGINE_H
#define HEURISTIC_ENGINE_H

#include "../grid/Grid3D.h"
#include <vector>
#include <map>

using namespace std;

struct HeuristicWeights {
    float w_distance;           // Distance to goal
    float w_police_proximity;   // Risk from nearby police
    float w_cctv_penalty;       // Penalty for CCTV zones
    float w_alert_zone_penalty; // Penalty for alert zones
    float w_vertical_cost;      // Cost of vertical movement
    float w_transition_control; // Police priority for transition choke points (US/DS)
    float w_transition_exposure; // Robber penalty when exposed on transition cells
    
    HeuristicWeights() 
        : w_distance(1.0f), 
          w_police_proximity(0.5f),
          w_cctv_penalty(2.0f), 
          w_alert_zone_penalty(1.0f),
          w_vertical_cost(1.5f),
          w_transition_control(2.0f),
          w_transition_exposure(2.0f) {}
};

class HeuristicEngine {
private:
    HeuristicWeights weights;
    const Grid3D& grid;
    
public:
    HeuristicEngine(const Grid3D& g);
    
    // Calculate weighted heuristic score
    float calculateHeuristic(const Position& current, 
                           const Position& goal, 
                           const vector<Position>& policePositions,
                           bool isRobberPerspective,
                           bool canTransitionFloors = true);
    
    // Individual scoring components
    float scoreDistance(const Position& current, const Position& goal);
    float scorePoliceProximity(const Position& current, const vector<Position>& police);
    float scoreCCTVRisk(const Position& current);
    float scoreAlertZoneRisk(const Position& current);
    float scoreVerticalCost(const Position& current, const Position& goal);
    float scoreTransitionControl(const Position& current,
                               const Position& goal,
                               bool canTransitionFloors);
    float scoreTransitionExposure(const Position& current,
                                 const vector<Position>& police);
    
    // Difficulty adjustment
    void adjustForDifficulty(int difficulty);
    void setWeights(const HeuristicWeights& w) { weights = w; }
    HeuristicWeights getWeights() const { return weights; }
};

#endif
