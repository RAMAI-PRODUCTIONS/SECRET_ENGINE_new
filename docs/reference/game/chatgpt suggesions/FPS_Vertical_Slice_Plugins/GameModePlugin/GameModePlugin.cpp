#include "GameModePlugin.h"
#include "SecretEngine/Logger.h"

void GameModePlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("GameModePlugin loaded");
}

void GameModePlugin::OnUpdate(float dt) {
    // TODO: Check win condition
}

void GameModePlugin::OnKill(int teamId) {
    m_teamScore[teamId]++;
}