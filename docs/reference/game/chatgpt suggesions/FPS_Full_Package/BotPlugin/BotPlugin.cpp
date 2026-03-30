#include "BotPlugin.h"
#include "../WeaponPlugin/WeaponPlugin.h"
#include "SecretEngine/Logger.h"

void BotPlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("BotPlugin loaded");

    for (int i=0;i<3;i++) {
        auto* b = m_core->GetWorld()->CreateEntity();
        b->position = Vec3(i*2,0,5);
        b->health = 100;
        b->team = 1;
    }
}

void BotPlugin::OnUpdate(float dt) {
    auto* weapon = m_core->GetPlugin<WeaponPlugin>();
    auto* player = m_core->GetWorld()->GetPlayer();

    for (auto* b : m_core->GetWorld()->GetEntities()) {
        if (!b->alive || b->team != 1) continue;

        Vec3 dir = (player->position - b->position).Norm();
        weapon->Fire(b->position, dir, 1);
    }
}