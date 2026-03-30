// SecretEngine
// Module: FPSUIPlugin
// Responsibility: FPS UI implementation with level switching
// Dependencies: Core, FPSGameLogic, LevelSystem

#include "FPSUIPlugin.h"
#include "../../LevelSystem/src/LevelSystemPlugin.h"

namespace SecretEngine::FPSUI {

void FPSUIPlugin::OnLoad(ICore* core) {
    m_core = core;
    m_logger = core->GetLogger();
    m_logger->LogInfo("FPSUI", "loaded");
    
    core->RegisterCapability("fps_ui", this);
}

void FPSUIPlugin::OnActivate() {
    m_logger->LogInfo("FPSUI", "activated");
    
    // Get level system
    auto* levelPlugin = static_cast<Levels::LevelSystemPlugin*>(
        m_core->GetCapability("level_system")
    );
    
    if (levelPlugin) {
        m_levelManager = levelPlugin->GetLevelManager();
        m_logger->LogInfo("FPSUI", "Level system connected");
        
        // Log available levels
        m_levelManager->PrintLevelInfo();
    } else {
        m_logger->LogWarning("FPSUI", "Level system not found");
    }
    
    // Initialize current level
    strncpy(m_currentLevel, "MainMenu", 64);
}

void FPSUIPlugin::OnDeactivate() {
    m_logger->LogInfo("FPSUI", "deactivated");
}

void FPSUIPlugin::OnUnload() {
    m_logger->LogInfo("FPSUI", "unloaded");
}

void FPSUIPlugin::OnUpdate(float dt) {
    // Update UI state
    m_uiTimer += dt;
    
    // Log UI state periodically
    if (m_uiTimer > 10.0f) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Current level: %s", m_currentLevel);
        m_logger->LogInfo("FPSUI", msg);
        m_uiTimer = 0.0f;
    }
}

void FPSUIPlugin::SwitchToLevel(const char* levelName) {
    if (!m_levelManager) {
        m_logger->LogError("FPSUI", "Cannot switch level: Level manager not available");
        return;
    }
    
    char msg[128];
    snprintf(msg, sizeof(msg), "🎮 Switching from %s to %s", m_currentLevel, levelName);
    m_logger->LogInfo("FPSUI", msg);
    
    // Handle current level
    if (strcmp(m_currentLevel, "MainMenu") == 0) {
        // Menu is persistent, just hide it
        m_levelManager->HideLevel(m_currentLevel);
    } else if (m_currentLevel[0] != '\0') {
        // Game levels are streaming, unload them
        m_levelManager->UnloadLevel(m_currentLevel);
    }
    
    // Load new level
    if (strcmp(levelName, "MainMenu") == 0) {
        // Menu is already loaded, just show it
        m_levelManager->ShowLevel(levelName);
    } else {
        // Load game level
        m_levelManager->LoadLevel(levelName, Levels::LoadPriority::Immediate);
    }
    
    // Update current level
    strncpy(m_currentLevel, levelName, 64);
    
    snprintf(msg, sizeof(msg), "✅ Now in level: %s", m_currentLevel);
    m_logger->LogInfo("FPSUI", msg);
}

void FPSUIPlugin::OnButtonPress(const char* buttonName) {
    char msg[128];
    snprintf(msg, sizeof(msg), "🔘 Button pressed: %s", buttonName);
    m_logger->LogInfo("FPSUI", msg);
    
    // Handle level switching buttons
    if (strcmp(buttonName, "MainMenu") == 0) {
        SwitchToLevel("MainMenu");
    }
    else if (strcmp(buttonName, "Scene") == 0) {
        SwitchToLevel("Scene");
    }
    else if (strcmp(buttonName, "FPS_Arena") == 0) {
        SwitchToLevel("FPS_Arena");
    }
    else if (strcmp(buttonName, "RacingTrack") == 0) {
        SwitchToLevel("RacingTrack");
    }
    else {
        snprintf(msg, sizeof(msg), "Unknown button: %s", buttonName);
        m_logger->LogWarning("FPSUI", msg);
    }
}

const char* FPSUIPlugin::GetCurrentLevel() const {
    return m_currentLevel;
}

} // namespace SecretEngine::FPSUI

extern "C" {
    SecretEngine::IPlugin* CreateFPSUIPlugin() {
        return new SecretEngine::FPSUI::FPSUIPlugin();
    }
}
