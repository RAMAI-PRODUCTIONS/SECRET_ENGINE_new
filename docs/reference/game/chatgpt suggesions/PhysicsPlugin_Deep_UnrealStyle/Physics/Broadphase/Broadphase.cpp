#include "Broadphase.h"

void Broadphase::ComputePairs(const std::vector<RigidBody*>& bodies,
                              std::vector<CollisionPair>& outPairs) {
    for (size_t i=0;i<bodies.size();i++)
        for (size_t j=i+1;j<bodies.size();j++)
            outPairs.push_back({ bodies[i], bodies[j] });
}