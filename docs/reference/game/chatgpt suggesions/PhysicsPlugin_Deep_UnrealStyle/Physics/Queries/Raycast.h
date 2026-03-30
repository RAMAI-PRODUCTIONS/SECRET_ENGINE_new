#pragma once
#include "../Bodies/RigidBody.h"

struct RaycastHit {
    bool hit=false;
    RigidBody* body=nullptr;
    float distance=0;
};

RaycastHit Raycast(const Vec3& o, const Vec3& d,
                   const std::vector<RigidBody*>& bodies);