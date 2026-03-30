#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include "PhysicsTypes.h"

class PhysicsPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnUpdate(float dt) override;

    RaycastHit Raycast(const Vec3& origin, const Vec3& dir, float maxDist);
    void Integrate(float dt);

private:
    ICore* m_core = nullptr;
};