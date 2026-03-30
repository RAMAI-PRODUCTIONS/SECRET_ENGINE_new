#include "PhysicsScene.h"

BodyID PhysicsScene::CreateBody(RigidBody* body) {
    body->id = (BodyID)bodies.size()+1;
    bodies.push_back(body);
    return body->id;
}

void PhysicsScene::Simulate(float dt) {
    for (auto* b : bodies) {
        if (b->type != EBodyType::Dynamic) continue;
        b->velocity.y -= 9.8f * dt;
        b->position = b->position + b->velocity * dt;
    }

    std::vector<CollisionPair> pairs;
    broadphase.ComputePairs(bodies, pairs);
    for (auto& p : pairs)
        solver.Resolve(p);
}