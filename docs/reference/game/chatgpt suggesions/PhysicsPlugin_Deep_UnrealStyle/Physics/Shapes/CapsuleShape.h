#pragma once
#include "Shape.h"

struct CapsuleShape : public Shape {
    float radius;
    float height;
    CapsuleShape(float r,float h):radius(r),height(h){
        type = EShapeType::Capsule;
    }
};