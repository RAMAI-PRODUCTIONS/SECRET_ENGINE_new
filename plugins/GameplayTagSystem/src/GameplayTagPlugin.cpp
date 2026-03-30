// SecretEngine
// Module: GameplayTagPlugin
// Responsibility: Plugin implementation for Gameplay Tag System
// Dependencies: Core, GameplayAbilitySystem

#include "GameplayTagPlugin.h"

namespace SecretEngine::GAS {

void GameplayTagPlugin::OnLoad(ICore* core) {
    m_core = core;
    m_world = core->GetWorld();
    m_logger = core->GetLogger();
    
    m_logger->LogInfo("GameplayTagSystem", "loaded");
    
    // Create data table
    m_dataTable = new GameplayDataTable(core);
    
    // Register capability
    core->RegisterCapability("gameplay_tags", this);
}

void GameplayTagPlugin::OnActivate() {
    m_logger->LogInfo("GameplayTagSystem", "activated");
    
    // Load data table
    bool loaded = m_dataTable->LoadDataTable("data/GameDataTable.json");
    if (loaded) {
        m_logger->LogInfo("GameplayTagSystem", "data table loaded successfully");
    } else {
        m_logger->LogError("GameplayTagSystem", "failed to load data table");
    }
}

void GameplayTagPlugin::OnDeactivate() {
    m_logger->LogInfo("GameplayTagSystem", "deactivated");
}

void GameplayTagPlugin::OnUnload() {
    if (m_dataTable) {
        delete m_dataTable;
        m_dataTable = nullptr;
    }
    m_logger->LogInfo("GameplayTagSystem", "unloaded");
}

void GameplayTagPlugin::OnUpdate(float dt) {
    // Update all entities with gameplay effects and abilities
    // This is a simplified update - in production you'd iterate entities more efficiently
    
    // For now, we'll let individual systems handle their own updates
    // The FPS systems will call into the tag system as needed
}

} // namespace SecretEngine::GAS

// Plugin factory implementation
extern "C" {
    SecretEngine::IPlugin* CreateGameplayTagPlugin() {
        return new SecretEngine::GAS::GameplayTagPlugin();
    }
    
    void DestroyGameplayTagPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
