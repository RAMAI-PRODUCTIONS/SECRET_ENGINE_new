#pragma once
#include "../Bodies/RigidBody.h"
#include "../Broadphase/Broadphase.h"
#include "../Solver/ImpulseSolver.h"
#include <vector>

class PhysicsScene {
public:
    BodyID CreateBody(RigidBody* body);
    void Simulate(float dt);

private:
    std::vector<RigidBody*> bodies;
    Broadphase broadphase;
    ImpulseSolver solver;
};