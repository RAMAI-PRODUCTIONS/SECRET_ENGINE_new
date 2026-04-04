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
    
    // Directly load the cyberpunk city level using LevelLoader
    m_logger->LogInfo("LevelSystem", "Loading cyberpunk city directly...");
    
    // Create a temporary level structure
    Level tempLevel;
    strncpy(tempLevel.definition.name, "CyberpunkCity", sizeof(tempLevel.definition.name) - 1);
    strncpy(tempLevel.definition.path, "levels/cyberpunk_city.json", sizeof(tempLevel.definition.path) - 1);
    tempLevel.state = LevelState::Unloaded;
    
    // Use LevelLoader to load the level data
    if (m_levelManager && m_levelManager->GetLevelLoader()) {
        bool loaded = m_levelManager->GetLevelLoader()->LoadLevelFromFile("levels/cyberpunk_city.json", &tempLevel);
        if (loaded) {
            m_logger->LogInfo("LevelSystem", "Cyberpunk city loaded successfully!");
        } else {
            m_logger->LogError("LevelSystem", "Failed to load cyberpunk city");
        }
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
        // Use basic Level System for simplicity
        return new SecretEngine::Levels::LevelSystemPlugin();
    }
    
    void DestroyLevelSystemPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
