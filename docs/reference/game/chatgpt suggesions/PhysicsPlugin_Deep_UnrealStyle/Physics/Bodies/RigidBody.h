#pragma once
#include "../Core/PhysicsCommon.h"
#include "../Shapes/Shape.h"

struct RigidBody {
    BodyID id;
    EBodyType type;
    Shape* shape;

    Vec3 position;
    Vec3 velocity;
    Vec3 force;

    float mass;
    bool awake = true;
};