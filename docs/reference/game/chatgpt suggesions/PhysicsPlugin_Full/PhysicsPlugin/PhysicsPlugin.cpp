#include "PhysicsPlugin.h"
#include "SecretEngine/Logger.h"

void PhysicsPlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("PhysicsPlugin loaded");
}

void PhysicsPlugin::OnUpdate(float dt) {
    Integrate(dt);
}

void PhysicsPlugin::Integrate(float dt) {
    for (auto* e : m_core->GetWorld()->GetEntities()) {
        if (!e->alive) continue;
        e->velocity.y -= 9.8f * dt;
        e->position = e->position + e->velocity * dt;
    }
}

RaycastHit PhysicsPlugin::Raycast(const Vec3& origin, const Vec3& dir, float maxDist) {
    RaycastHit out;
    float best = maxDist;

    for (auto* e : m_core->GetWorld()->GetEntities()) {
        if (!e->alive) continue;

        Vec3 to = e->position - origin;
        float proj = to.Dot(dir);
        if (proj < 0 || proj > best) continue;

        Vec3 closest = origin + dir * proj;
        float dist = (e->position - closest).Len();

        if (dist < 0.5f) {
            best = proj;
            out.hit = true;
            out.distance = proj;
            out.point = closest;
            out.entity = e;
        }
    }
    return out;
}