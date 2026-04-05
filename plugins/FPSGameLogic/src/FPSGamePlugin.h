// SecretEngine
// Module: FPSGamePlugin
// Responsibility: FPS game logic orchestration
// Dependencies: Core, PhysicsPlugin

#pragma once
#include "SecretEngine/IPlugin.h"
#include "SecretEngine/ICore.h"
#include "SecretEngine/IWorld.h"
#include "SecretEngine/ILogger.h"
#include "SecretEngine/ILightingSystem.h"
#include "SecretEngine/IRenderer.h"
#include "SecretEngine/JobSystem.h"
#include "SecretEngine/Components.h"
#include "FPSComponents.h"
#include "FPSFastData.h"

// Forward declare PhysicsPlugin
namespace SecretEngine::Physics {
    class PhysicsPlugin;
}

namespace SecretEngine::FPS {

// Forward declare systems
namespace MovementSystem {
    void Update(IWorld* world, SecretEngine::Physics::PhysicsPlugin* physics, float deltaTime, 
                Fast::FastPacketQueue<Fast::PlayerMovePacket>& moveQueue);
}

namespace WeaponSystem {
    void Update(IWorld* world, SecretEngine::Physics::PhysicsPlugin* physics, float currentTime,
                Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue,
                Fast::FastPacketQueue<Fast::DamageEventPacket>& damageQueue,
                ILogger* logger);
}

namespace CombatSystem {
    void ProcessDamageEvents(IWorld* world, Fast::FastPacketQueue<Fast::DamageEventPacket>& damageQueue,
                            MatchState& matchState);
}

namespace AISystem {
    void Update(IWorld* world, float deltaTime, Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue,
                Entity playerEntity);
}

class FPSGamePlugin : public IPlugin {
public:
    // IPlugin interface
    const char* GetName() const override { return "FPSGameLogic"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t id) override { 
        if (id == 100) return this; // FPS Game interface
        return nullptr; 
    }
    
    // API for other plugins
    Fast::FPSFastStreams& GetFastStreams() { return m_fastStreams; }
    Entity GetLocalPlayerEntity() const { return m_localPlayerEntity; }
    const MatchState& GetMatchState() const { return m_matchState; }
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
    
    MatchState m_matchState;
    Fast::FPSFastStreams m_fastStreams;
    Entity m_localPlayerEntity = Entity::Invalid;
    float m_gameTime = 0.0f;
    
    // Forward+ inspired: Dynamic moving lights
    struct MovingLight {
        uint32_t lightID;
        uint32_t particleInstanceID;  // visual particle in the renderer
        float position[3];
        float velocity[3];
        float color[3];
        float radius;
        float speed;
        float rotPhase;  // for spinning animation
    };
    std::vector<MovingLight> m_movingLights;
    
    // Particle mesh path (small cube used as light orb)
    static constexpr const char* PARTICLE_MESH = "meshes/Character.meshbin";
    static constexpr float PARTICLE_SCALE = 8.0f;  // small orb
    
    void RegisterComponents();
    void CreatePlayerEntity();
    void CreateBotEntities(int count);
    
    // Forward+ inspired: Dynamic lighting
    void CreateRandomMovingLights(int count);
    void UpdateMovingLights(float deltaTime);
    void SpawnLightParticles(SecretEngine::IRenderer* renderer);

    bool m_particlesSpawned = false;
};

} // namespace SecretEngine::FPS

// Plugin factory
extern "C" {
    SecretEngine::IPlugin* CreateFPSGamePlugin();
    void DestroyFPSGamePlugin(SecretEngine::IPlugin* plugin);
}
