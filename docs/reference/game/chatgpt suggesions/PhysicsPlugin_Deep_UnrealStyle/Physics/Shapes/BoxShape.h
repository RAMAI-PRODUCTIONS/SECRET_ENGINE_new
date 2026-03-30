#pragma once
#include "Shape.h"

struct BoxShape : public Shape {
    Vec3 halfExtents;
    BoxShape(const Vec3& he):halfExtents(he){
        type = EShapeType::Box;
    }
};