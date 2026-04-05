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
    
    // Deferred particle spawn: renderer wasn't ready during OnActivate
    if (!m_particlesSpawned) {
        auto* rendererPlugin = m_core->GetCapability("rendering");
        if (rendererPlugin) {
            auto* renderer = static_cast<SecretEngine::IRenderer*>(rendererPlugin->GetInterface(1));
            if (renderer) {
                SpawnLightParticles(renderer);
                m_particlesSpawned = true;
            }
        }
    }
    
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

    // Get renderer for particle instances (unused here - deferred to SpawnLightParticles)
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-15.0f, 15.0f);
    std::uniform_real_distribution<float> heightDist(1.0f, 5.0f);
    std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
    std::uniform_real_distribution<float> speedDist(1.0f, 3.0f);
    std::uniform_real_distribution<float> radiusDist(3.0f, 8.0f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 6.28318f);
    
    for (int i = 0; i < count; ++i) {
        SecretEngine::LightData light;
        light.type = SecretEngine::LightData::Point;
        
        float x = posDist(gen);
        float y = heightDist(gen);
        float z = posDist(gen);
        
        light.position[0] = x;
        light.position[1] = y;
        light.position[2] = z;
        
        light.color[0] = colorDist(gen);
        light.color[1] = colorDist(gen);
        light.color[2] = colorDist(gen);
        
        light.intensity = 2.0f;
        light.range = radiusDist(gen);
        light.constantAttenuation = 1.0f;
        light.linearAttenuation = 0.09f;
        light.quadraticAttenuation = 0.032f;
        
        uint32_t lightID = lightingSystem->AddLight(light);
        
        // Spawn a small glowing particle at the light position
        uint32_t particleID = UINT32_MAX;
        // Note: particles are spawned deferred in SpawnLightParticles() once renderer is ready
        
        MovingLight movingLight;
        movingLight.lightID = lightID;
        movingLight.particleInstanceID = particleID;
        movingLight.position[0] = x;
        movingLight.position[1] = y;
        movingLight.position[2] = z;
        movingLight.color[0] = light.color[0];
        movingLight.color[1] = light.color[1];
        movingLight.color[2] = light.color[2];
        movingLight.radius = light.range;
        movingLight.speed = speedDist(gen);
        movingLight.rotPhase = phaseDist(gen);
        
        std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);
        movingLight.velocity[0] = velDist(gen);
        movingLight.velocity[1] = velDist(gen) * 0.5f;
        movingLight.velocity[2] = velDist(gen);
        
        m_movingLights.push_back(movingLight);
    }
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "created %d light particles (mesh orbs)", count);
    m_logger->LogInfo("FPSGameLogic", buffer);
}

// Deferred particle spawn - called once renderer is ready
void FPSGamePlugin::SpawnLightParticles(SecretEngine::IRenderer* renderer) {
    for (auto& light : m_movingLights) {
        if (light.particleInstanceID == UINT32_MAX) {
            light.particleInstanceID = renderer->SpawnInstance(
                PARTICLE_MESH,
                light.position[0], light.position[1], light.position[2],
                light.color[0], light.color[1], light.color[2],
                PARTICLE_SCALE
            );
        }
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "spawned %zu light particle instances", m_movingLights.size());
    m_logger->LogInfo("FPSGameLogic", buf);
}

// Parallel light simulation using JobSystem + FDA packets
void FPSGamePlugin::UpdateMovingLights(float deltaTime) {
    if (m_movingLights.empty()) return;

    auto* lightingPlugin = m_core->GetCapability("lighting");
    if (!lightingPlugin) return;
    auto* lightingSystem = static_cast<SecretEngine::ILightingSystem*>(lightingPlugin->GetInterface(3));
    if (!lightingSystem) return;

    uint32_t count = (uint32_t)m_movingLights.size();

    // Use ParallelFor to simulate lights on worker threads
    SecretEngine::ParallelFor(count, [this, deltaTime, lightingSystem](uint32_t i) {
        auto& light = m_movingLights[i];

        // Move
        light.position[0] += light.velocity[0] * light.speed * deltaTime;
        light.position[1] += light.velocity[1] * light.speed * deltaTime;
        light.position[2] += light.velocity[2] * light.speed * deltaTime;

        // Bounce
        const float B = 15.0f;
        if (light.position[0] < -B || light.position[0] > B) { light.velocity[0] *= -1.f; light.position[0] = std::clamp(light.position[0], -B, B); }
        if (light.position[1] < 1.f || light.position[1] > 8.f) { light.velocity[1] *= -1.f; light.position[1] = std::clamp(light.position[1], 1.f, 8.f); }
        if (light.position[2] < -B || light.position[2] > B) { light.velocity[2] *= -1.f; light.position[2] = std::clamp(light.position[2], -B, B); }

        // Pulse colour
        light.rotPhase += deltaTime * 1.5f;
        float pulse = 0.7f + 0.3f * sinf(light.rotPhase);
        float pr = light.color[0] * pulse;
        float pg = light.color[1] * pulse;
        float pb = light.color[2] * pulse;

        // Push FDA packet to MegaLightRenderer (lock-free, ~50ns)
        lightingSystem->UpdateLight(light.lightID, [&]() -> SecretEngine::LightData {
            SecretEngine::LightData d;
            d.type = SecretEngine::LightData::Point;
            d.position[0] = light.position[0]; d.position[1] = light.position[1]; d.position[2] = light.position[2];
            d.color[0] = pr; d.color[1] = pg; d.color[2] = pb;
            d.intensity = 2.0f; d.range = light.radius;
            d.constantAttenuation = 1.0f; d.linearAttenuation = 0.09f; d.quadraticAttenuation = 0.032f;
            return d;
        }());
    });

    // Update particle visuals on main thread (renderer calls are not thread-safe)
    auto* rendererPlugin = m_core->GetCapability("rendering");
    SecretEngine::IRenderer* renderer = rendererPlugin
        ? static_cast<SecretEngine::IRenderer*>(rendererPlugin->GetInterface(1)) : nullptr;
    if (renderer) {
        for (auto& light : m_movingLights) {
            if (light.particleInstanceID == UINT32_MAX) continue;
            float pulse = 0.7f + 0.3f * sinf(light.rotPhase);
            renderer->UpdateInstancePosColor(light.particleInstanceID,
                light.position[0], light.position[1], light.position[2],
                light.color[0]*pulse, light.color[1]*pulse, light.color[2]*pulse,
                PARTICLE_SCALE);
        }
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
