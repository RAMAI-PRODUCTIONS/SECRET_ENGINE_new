#pragma once
#include "../Common/Math.h"

struct AABB {
    Vec3 min;
    Vec3 max;

    bool Intersects(const AABB& o) const {
        return (min.x <= o.max.x && max.x >= o.min.x) &&
               (min.y <= o.max.y && max.y >= o.min.y) &&
               (min.z <= o.max.z && max.z >= o.min.z);
    }
};

struct RaycastHit {
    bool hit = false;
    Vec3 point;
    float distance = 0.0f;
    void* entity = nullptr;
};