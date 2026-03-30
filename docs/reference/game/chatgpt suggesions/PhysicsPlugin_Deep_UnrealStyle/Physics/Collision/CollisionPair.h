#pragma once
#include "../Bodies/RigidBody.h"

struct CollisionPair {
    RigidBody* a;
    RigidBody* b;
};