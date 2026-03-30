// SecretEngine
// Module: LevelSystemPlugin
// Responsibility: Plugin wrapper for Level System
// Dependencies: LevelManager

#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include "LevelManager.h"

namespace SecretEngine::Levels {

class LevelSystemPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t interfaceId) override { return nullptr; }
    const char* GetName() const override { return "LevelSystem"; }
    uint32_t GetVersion() const override { return 1; }
    
    // Access to level manager
    LevelManager* GetLevelManager() { return m_levelManager; }
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
    LevelManager* m_levelManager = nullptr;
    
    float m_playerPosition[3] = {0, 0, 0};
};

} // namespace SecretEngine::Levels
