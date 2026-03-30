#include "NarrowPhase.h"
#include <cmath>

bool TestSphereSphere(const RigidBody& a, const RigidBody& b) {
    auto* sa = (SphereShape*)a.shape;
    auto* sb = (SphereShape*)b.shape;
    float dist = (a.position - b.position).Len();
    return dist < (sa->radius + sb->radius);
}