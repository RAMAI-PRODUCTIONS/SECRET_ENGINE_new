#include "WeaponPlugin.h"
#include "SecretEngine/Logger.h"

void WeaponPlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("WeaponPlugin loaded");
}

void WeaponPlugin::OnUpdate(float dt) {
    m_cooldown -= dt;
}

void WeaponPlugin::Fire(const Vec3& origin, const Vec3& dir, int team) {
    if (m_cooldown > 0) return;
    m_cooldown = 0.2f;

    // Very simple hitscan against bots + player
    auto entities = m_core->GetWorld()->GetEntities();
    float bestDist = 10000.0f;
    Entity* hit = nullptr;

    for (auto* e : entities) {
        if (e->team == team) continue;
        Vec3 to = e->position - origin;
        float proj = to.Dot(dir);
        if (proj < 0) continue;

        Vec3 closest = origin + dir * proj;
        float dist = (e->position - closest).Len();
        if (dist < 0.5f && proj < bestDist) {
            bestDist = proj;
            hit = e;
        }
    }

    if (hit) {
        hit->health -= 50;
        Logger::Info("Hit entity");
        if (hit->health <= 0) {
            m_core->GetPlugin<GameModePlugin>()->OnKill(team);
            hit->alive = false;
        }
    }
}