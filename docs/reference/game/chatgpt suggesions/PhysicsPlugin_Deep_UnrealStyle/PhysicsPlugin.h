#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include "Physics/Scene/PhysicsScene.h"

class PhysicsPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnUpdate(float dt) override;

    PhysicsScene& GetScene();

private:
    ICore* m_core=nullptr;
    PhysicsScene scene;
};