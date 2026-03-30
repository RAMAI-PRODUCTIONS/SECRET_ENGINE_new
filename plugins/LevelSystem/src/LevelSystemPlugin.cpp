// SecretEngine
// Module: LevelSystemPlugin
// Responsibility: Plugin implementation for Level System
// Dependencies: LevelManager

#include "LevelSystemPlugin.h"
#include <SecretEngine/Components.h>

namespace SecretEngine::Levels {

void LevelSystemPlugin::OnLoad(ICore* core) {
    m_core = core;
    m_world = core->GetWorld();
    m_logger = core->GetLogger();
    
    m_logger->LogInfo("LevelSystem", "loaded");
    
    // Create level manager
    m_levelManager = new LevelManager(core);
    
    // Register capability
    core->RegisterCapability("level_system", this);
}

void LevelSystemPlugin::OnActivate() {
    m_logger->LogInfo("LevelSystem", "activated");
    
    // Load level definitions
    bool loaded = m_levelManager->LoadLevelDefinitions("data/LevelDefinitions.json");
    if (loaded) {
        m_logger->LogInfo("LevelSystem", "level definitions loaded successfully");
        m_levelManager->PrintLevelInfo();
    } else {
        m_logger->LogWarning("LevelSystem", "no level definitions found (optional)");
    }
}

void LevelSystemPlugin::OnDeactivate() {
    m_logger->LogInfo("LevelSystem", "deactivated");
}

void LevelSystemPlugin::OnUnload() {
    if (m_levelManager) {
        delete m_levelManager;
        m_levelManager = nullptr;
    }
    m_logger->LogInfo("LevelSystem", "unloaded");
}

void LevelSystemPlugin::OnUpdate(float dt) {
    if (!m_levelManager) return;
    
    // Get player position (simplified - in production, query from player entity)
    // For now, assume player is at origin
    m_playerPosition[0] = 0;
    m_playerPosition[1] = 0;
    m_playerPosition[2] = 0;
    
    // Update level streaming
    m_levelManager->UpdateStreaming(dt, m_playerPosition);
}

} // namespace SecretEngine::Levels

// Plugin factory implementation
extern "C" {
    SecretEngine::IPlugin* CreateLevelSystemPlugin() {
        return new SecretEngine::Levels::LevelSystemPlugin();
    }
    
    void DestroyLevelSystemPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
