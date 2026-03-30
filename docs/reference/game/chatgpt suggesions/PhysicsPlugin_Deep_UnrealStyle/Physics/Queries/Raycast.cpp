#include "Raycast.h"

RaycastHit Raycast(const Vec3& o, const Vec3& d,
                   const std::vector<RigidBody*>& bodies) {
    RaycastHit out;
    float best = 1e9f;

    for (auto* b : bodies) {
        Vec3 to = b->position - o;
        float proj = to.Dot(d);
        if (proj < 0 || proj > best) continue;

        float dist = (to - d*proj).Len();
        if (dist < 0.5f) {
            best = proj;
            out.hit = true;
            out.body = b;
            out.distance = proj;
        }
    }
    return out;
}