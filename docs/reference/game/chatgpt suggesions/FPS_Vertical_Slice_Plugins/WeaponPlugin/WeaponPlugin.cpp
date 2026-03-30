#include "WeaponPlugin.h"
#include "SecretEngine/Logger.h"

void WeaponPlugin::OnLoad(ICore* core) {
    m_core = core;
    Logger::Info("WeaponPlugin loaded");
}

void WeaponPlugin::OnUpdate(float dt) {
    // TODO: check input and call Fire()
}

void WeaponPlugin::Fire() {
    // TODO: Raycast from camera and apply damage
}