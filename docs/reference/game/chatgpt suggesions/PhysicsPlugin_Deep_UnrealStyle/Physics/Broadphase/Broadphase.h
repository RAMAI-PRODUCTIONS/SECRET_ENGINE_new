#pragma once
#include "../Bodies/RigidBody.h"
#include <vector>

class Broadphase {
public:
    void ComputePairs(const std::vector<RigidBody*>& bodies,
                      std::vector<CollisionPair>& outPairs);
};