#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include "../Common/Math.h"

class WeaponPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnUpdate(float dt) override;

    void Fire(const Vec3& origin, const Vec3& dir, int team);

private:
    ICore* m_core = nullptr;
    float m_cooldown = 0.0f;
};