#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"

class GameModePlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnUpdate(float dt) override;
    void OnKill(int killerTeam);

private:
    ICore* m_core = nullptr;
    int score[2] = {0,0};
};