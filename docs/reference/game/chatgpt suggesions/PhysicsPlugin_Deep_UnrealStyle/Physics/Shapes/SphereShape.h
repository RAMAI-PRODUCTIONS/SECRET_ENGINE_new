#pragma once
#include "Shape.h"

struct SphereShape : public Shape {
    float radius;
    SphereShape(float r) : radius(r) { type = EShapeType::Sphere; }
};