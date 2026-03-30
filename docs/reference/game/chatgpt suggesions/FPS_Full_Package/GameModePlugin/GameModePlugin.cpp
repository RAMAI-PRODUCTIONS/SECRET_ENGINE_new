#include "GameModePlugin.h"
#include "SecretEngine/Logger.h"

void GameModePlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("GameModePlugin loaded");
}

void GameModePlugin::OnUpdate(float dt) {
    if (score[0] >= 10 || score[1] >= 10) {
        Logger::Info("Match over");
    }
}

void GameModePlugin::OnKill(int team) {
    score[team]++;
    Logger::Info("Kill registered");
}