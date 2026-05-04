#pragma once

#include "../grid/Grid3D.h"

namespace ai {
namespace planner {

struct Node {
    Position pos;
    int g;
    int h;
    int f;
    bool chosen;

    Node() : pos(), g(0), h(0), f(0), chosen(false) {}
};

} // namespace planner
} // namespace ai
