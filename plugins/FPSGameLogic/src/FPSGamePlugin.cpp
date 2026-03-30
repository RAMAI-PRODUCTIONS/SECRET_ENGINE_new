// SecretEngine
// Module: FPSGamePlugin
// Responsibility: FPS game plugin implementation
// Dependencies: Core, PhysicsPlugin, GameplayTagSystem

#include "FPSGamePlugin.h"
#include "../../PhysicsPlugin/src/PhysicsPlugin.h"
#include "../../GameplayTagSystem/src/GameplayTagPlugin.h"
#include <random>

namespace SecretEngine::FPS {

void FPSGamePlugin::OnLoad(ICore* core) {
    m_core = core;
    m_world = core->GetWorld();
    m_logger = core->GetLogger();
    
    m_logger->LogInfo("FPSGameLogic", "loaded");
    
    RegisterComponents();
    
    // Register capability
    core->RegisterCapability("fps_game", this);
}

void FPSGamePlugin::OnActivate() {
    m_logger->LogInfo("FPSGameLogic", "activated");
    
    // Create player and bots
    CreatePlayerEntity();
    CreateBotEntities(5);
    
    m_logger->LogInfo("FPSGameLogic", "initialized: 1 player + 5 bots");
}

void FPSGamePlugin::OnDeactivate() {
    m_logger->LogInfo("FPSGameLogic", "deactivated");
}

void FPSGamePlugin::OnUnload() {
    m_logger->LogInfo("FPSGameLogic", "unloaded");
}

void FPSGamePlugin::OnUpdate(float dt) {
    m_gameTime += dt;
    m_matchState.matchTime += dt;
    
    // Get physics plugin
    auto* physics = static_cast<SecretEngine::Physics::PhysicsPlugin*>(
        m_core->GetCapability("physics")
    );
    
    if (!physics) return;
    
    // Update all systems
    MovementSystem::Update(m_world, physics, dt, m_fastStreams.moveInput);
    WeaponSystem::Update(m_world, physics, m_gameTime, m_fastStreams.actionInput, m_fastStreams.damageEvents, m_logger);
    CombatSystem::ProcessDamageEvents(m_world, m_fastStreams.damageEvents, m_matchState);
    AISystem::Update(m_world, dt, m_fastStreams.actionInput, m_localPlayerEntity);
    
    // Check win condition
    if (m_matchState.playerKills >= m_matchState.killLimit && !m_matchState.matchEnded) {
        m_matchState.matchEnded = true;
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "match ended! player wins with %d kills", m_matchState.playerKills);
        m_logger->LogInfo("FPSGameLogic", buffer);
    }
}

void FPSGamePlugin::RegisterComponents() {
    // Register FPS components with core
    m_logger->LogInfo("FPSGameLogic", "registering components");
    
    // Components are registered via template system
    // This is handled by the ECS in core
}

void FPSGamePlugin::CreatePlayerEntity() {
    // Get GameplayTagSystem plugin
    auto* tagPlugin = static_cast<SecretEngine::GAS::GameplayTagPlugin*>(
        m_core->GetCapability("gameplay_tags")
    );
    
    if (!tagPlugin) {
        m_logger->LogError("FPSGameLogic", "GameplayTagSystem not found! Using fallback.");
        // Fallback: create basic entity
        m_localPlayerEntity = m_world->CreateEntity();
        return;
    }
    
    auto* dataTable = tagPlugin->GetDataTable();
    
    // Create player from data table - ONE LINE!
    m_localPlayerEntity = dataTable->CreateCharacterFromTable("player_soldier");
    
    // Add KCC component (not in tag system yet)
    auto* kcc = new KCCComponent();
    kcc->moveSpeed = 5.0f;
    kcc->jumpForce = 7.0f;
    kcc->isGrounded = true;
    m_world->AddComponent(m_localPlayerEntity, KCCComponent::TypeID, kcc);
    
    m_logger->LogInfo("FPSGameLogic", "player entity created from data table (player_soldier)");
}

void FPSGamePlugin::CreateBotEntities(int count) {
    // Get GameplayTagSystem plugin
    auto* tagPlugin = static_cast<SecretEngine::GAS::GameplayTagPlugin*>(
        m_core->GetCapability("gameplay_tags")
    );
    
    if (!tagPlugin) {
        m_logger->LogError("FPSGameLogic", "GameplayTagSystem not found! Cannot create bots.");
        return;
    }
    
    auto* dataTable = tagPlugin->GetDataTable();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-10.0f, 10.0f);
    
    for (int i = 0; i < count; ++i) {
        // Create bot from data table - ONE LINE!
        Entity botId = dataTable->CreateCharacterFromTable("enemy_soldier");
        
        // Randomize position
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(botId, TransformComponent::TypeID)
        );
        if (transform) {
            transform->position[0] = posDist(gen);
            transform->position[2] = posDist(gen);
        }
        
        // Add KCC component (not in tag system yet)
        auto* kcc = new KCCComponent();
        kcc->moveSpeed = 3.0f;
        kcc->isGrounded = true;
        m_world->AddComponent(botId, KCCComponent::TypeID, kcc);
        
        // Set patrol target for AI
        auto* ai = static_cast<AIComponent*>(
            m_world->GetComponent(botId, AIComponent::TypeID)
        );
        if (ai) {
            ai->patrolTarget[0] = posDist(gen);
            ai->patrolTarget[1] = 0;
            ai->patrolTarget[2] = posDist(gen);
        }
    }
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "created %d bot entities from data table (enemy_soldier)", count);
    m_logger->LogInfo("FPSGameLogic", buffer);
}

} // namespace SecretEngine::FPS

// Plugin factory implementation
extern "C" {
    SecretEngine::IPlugin* CreateFPSGamePlugin() {
        return new SecretEngine::FPS::FPSGamePlugin();
    }
    
    void DestroyFPSGamePlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
