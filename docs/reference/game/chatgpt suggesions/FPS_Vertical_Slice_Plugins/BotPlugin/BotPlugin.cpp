#include "BotPlugin.h"
#include "SecretEngine/Logger.h"

void BotPlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("BotPlugin loaded");
    // TODO: Spawn bots
}

void BotPlugin::OnUpdate(float dt) {
    UpdateBots(dt);
}

void BotPlugin::UpdateBots(float dt) {
    // TODO: Simple AI (move + shoot)
}