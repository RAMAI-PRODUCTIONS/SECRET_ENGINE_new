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
    
    // Forward+ inspired: Create randomly moving lights to test the enhanced lighting system
    CreateRandomMovingLights(20);
    
    m_logger->LogInfo("FPSGameLogic", "initialized: 1 player + 5 bots + 20 moving lights");
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
    
    // Forward+ inspired: Update moving lights
    UpdateMovingLights(dt);
    
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

// Forward+ inspired: Create randomly moving lights to showcase the enhanced lighting system
void FPSGamePlugin::CreateRandomMovingLights(int count) {
    auto* lightingPlugin = m_core->GetCapability("lighting");
    
    if (!lightingPlugin) {
        m_logger->LogError("FPSGameLogic", "LightingSystem not found! Cannot create lights.");
        return;
    }
    
    auto* lightingSystem = static_cast<SecretEngine::ILightingSystem*>(lightingPlugin->GetInterface(3));
    if (!lightingSystem) {
        m_logger->LogError("FPSGameLogic", "LightingSystem GetInterface(3) failed!");
        return;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-15.0f, 15.0f);
    std::uniform_real_distribution<float> heightDist(1.0f, 5.0f);
    std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
    std::uniform_real_distribution<float> speedDist(1.0f, 3.0f);
    std::uniform_real_distribution<float> radiusDist(3.0f, 8.0f);
    
    for (int i = 0; i < count; ++i) {
        SecretEngine::LightData light;
        light.type = SecretEngine::LightData::Point;
        
        // Random position
        float x = posDist(gen);
        float y = heightDist(gen);
        float z = posDist(gen);
        
        light.position[0] = x;
        light.position[1] = y;
        light.position[2] = z;
        
        // Random color (vibrant colors)
        light.color[0] = colorDist(gen);
        light.color[1] = colorDist(gen);
        light.color[2] = colorDist(gen);
        
        // Forward+ inspired: Physically-based attenuation parameters
        light.intensity = 2.0f;
        light.range = radiusDist(gen);
        light.constantAttenuation = 1.0f;
        light.linearAttenuation = 0.09f;
        light.quadraticAttenuation = 0.032f;
        
        // Add light to system
        uint32_t lightID = lightingSystem->AddLight(light);
        
        // Store moving light data
        MovingLight movingLight;
        movingLight.lightID = lightID;
        movingLight.position[0] = x;
        movingLight.position[1] = y;
        movingLight.position[2] = z;
        movingLight.color[0] = light.color[0];
        movingLight.color[1] = light.color[1];
        movingLight.color[2] = light.color[2];
        movingLight.radius = light.range;
        movingLight.speed = speedDist(gen);
        
        // Random velocity
        std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);
        movingLight.velocity[0] = velDist(gen);
        movingLight.velocity[1] = velDist(gen) * 0.5f; // Less vertical movement
        movingLight.velocity[2] = velDist(gen);
        
        m_movingLights.push_back(movingLight);
    }
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "created %d randomly moving lights with Forward+ attenuation", count);
    m_logger->LogInfo("FPSGameLogic", buffer);
}

// Forward+ inspired: Update moving lights each frame
void FPSGamePlugin::UpdateMovingLights(float deltaTime) {
    auto* lightingPlugin = m_core->GetCapability("lighting");
    if (!lightingPlugin) return;
    auto* lightingSystem = static_cast<SecretEngine::ILightingSystem*>(lightingPlugin->GetInterface(3));
    if (!lightingSystem) return;
    
    for (auto& light : m_movingLights) {
        // Update position
        light.position[0] += light.velocity[0] * light.speed * deltaTime;
        light.position[1] += light.velocity[1] * light.speed * deltaTime;
        light.position[2] += light.velocity[2] * light.speed * deltaTime;
        
        // Bounce off boundaries
        const float boundary = 15.0f;
        if (light.position[0] < -boundary || light.position[0] > boundary) {
            light.velocity[0] *= -1.0f;
            light.position[0] = std::clamp(light.position[0], -boundary, boundary);
        }
        if (light.position[1] < 1.0f || light.position[1] > 8.0f) {
            light.velocity[1] *= -1.0f;
            light.position[1] = std::clamp(light.position[1], 1.0f, 8.0f);
        }
        if (light.position[2] < -boundary || light.position[2] > boundary) {
            light.velocity[2] *= -1.0f;
            light.position[2] = std::clamp(light.position[2], -boundary, boundary);
        }
        
        // Update light in system
        SecretEngine::LightData updatedLight;
        updatedLight.type = SecretEngine::LightData::Point;
        updatedLight.position[0] = light.position[0];
        updatedLight.position[1] = light.position[1];
        updatedLight.position[2] = light.position[2];
        updatedLight.color[0] = light.color[0];
        updatedLight.color[1] = light.color[1];
        updatedLight.color[2] = light.color[2];
        updatedLight.intensity = 2.0f;
        updatedLight.range = light.radius;
        updatedLight.constantAttenuation = 1.0f;
        updatedLight.linearAttenuation = 0.09f;
        updatedLight.quadraticAttenuation = 0.032f;
        
        lightingSystem->UpdateLight(light.lightID, updatedLight);
    }
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
