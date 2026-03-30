#include "ImpulseSolver.h"

void ImpulseSolver::Resolve(const CollisionPair& pair) {
    auto& a = *pair.a;
    auto& b = *pair.b;

    Vec3 normal = (b.position - a.position).Norm();
    float relVel = (b.velocity - a.velocity).Dot(normal);

    if (relVel > 0) return;

    float impulse = -relVel;
    a.velocity = a.velocity - normal * impulse * 0.5f;
    b.velocity = b.velocity + normal * impulse * 0.5f;
}