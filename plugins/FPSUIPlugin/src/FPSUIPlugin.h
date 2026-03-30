// SecretEngine
// Module: FPSUIPlugin
// Responsibility: FPS game UI overlay with level switching
// Dependencies: Core, FPSGameLogic, LevelSystem

#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include "SecretEngine/ILogger.h"

// Forward declaration
namespace SecretEngine::Levels {
    class LevelManager;
}

namespace SecretEngine::FPSUI {

class FPSUIPlugin : public IPlugin {
public:
    const char* GetName() const override { return "FPSUI"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t id) override { return nullptr; }
    
    // Level switching
    void SwitchToLevel(const char* levelName);
    void OnButtonPress(const char* buttonName);
    const char* GetCurrentLevel() const;
    
private:
    ICore* m_core = nullptr;
    ILogger* m_logger = nullptr;
    Levels::LevelManager* m_levelManager = nullptr;
    
    char m_currentLevel[64] = "";
    float m_uiTimer = 0.0f;
};

} // namespace SecretEngine::FPSUI

extern "C" {
    SecretEngine::IPlugin* CreateFPSUIPlugin();
}
