#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"

class WeaponPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnUpdate(float dt) override;
private:
    ICore* m_core = nullptr;
    void Fire();
};