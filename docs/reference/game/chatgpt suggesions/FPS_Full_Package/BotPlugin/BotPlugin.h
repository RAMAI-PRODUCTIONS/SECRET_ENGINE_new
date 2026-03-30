#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include "../Common/Math.h"

class BotPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnUpdate(float dt) override;

private:
    ICore* m_core = nullptr;
};