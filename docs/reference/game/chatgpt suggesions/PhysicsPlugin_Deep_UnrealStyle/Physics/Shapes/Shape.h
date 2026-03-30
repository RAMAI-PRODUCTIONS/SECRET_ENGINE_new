#pragma once
#include "../Core/PhysicsCommon.h"

enum class EShapeType {
    Sphere,
    Capsule,
    Box
};

struct Shape {
    EShapeType type;
    virtual ~Shape() = default;
};