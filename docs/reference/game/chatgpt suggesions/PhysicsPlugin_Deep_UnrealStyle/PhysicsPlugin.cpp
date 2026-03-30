#include "PhysicsPlugin.h"
#include "SecretEngine/Logger.h"

void PhysicsPlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("Deep PhysicsPlugin loaded");
}

void PhysicsPlugin::OnUpdate(float dt) {
    scene.Simulate(dt);
}

PhysicsScene& PhysicsPlugin::GetScene() {
    return scene;
}