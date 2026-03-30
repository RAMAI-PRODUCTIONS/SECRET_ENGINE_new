// SecretEngine
// Module: PhysicsPlugin
// Responsibility: Comprehensive physics system
// Dependencies: Core, PhysicsTypes, CollisionDetection

#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/Components.h>
#include "PhysicsTypes.h"
#include <vector>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine::Physics {

class PhysicsPlugin : public IPlugin {
public:
    // IPlugin interface
    const char* GetName() const override { return "Physics"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t id) override { return nullptr; }
    
    // Physics API - Raycasting (C++26: std::span for arrays)
    RaycastHit Raycast(std::span<const float, 3> origin, std::span<const float, 3> direction, float maxDistance);
    RaycastHit RaycastWithMask(std::span<const float, 3> origin, std::span<const float, 3> direction, 
                               float maxDistance, uint32_t layerMask);
    
    // Physics API - Queries
    bool CheckGround(std::span<const float, 3> position, float radius, float checkDistance);
    bool OverlapSphere(std::span<const float, 3> position, float radius, uint32_t layerMask = CollisionLayer::All);
    bool OverlapBox(std::span<const float, 3> position, std::span<const float, 3> halfExtents, uint32_t layerMask = CollisionLayer::All);
    
    // Physics API - Forces
    void AddForce(Entity entity, std::span<const float, 3> force);
    void AddImpulse(Entity entity, std::span<const float, 3> impulse);
    void SetVelocity(Entity entity, std::span<const float, 3> velocity);
    void AddExplosionForce(std::span<const float, 3> position, float force, float radius);
    
    // Physics API - Gravity
    void SetGravity(std::span<const float, 3> gravity);
    std::span<const float, 3> GetGravity() const { return m_gravity; }
    
    // Physics API - Collision filtering
    bool ShouldCollide(uint32_t layerA, uint32_t maskA, uint32_t layerB, uint32_t maskB) const;
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
    
    // Physics settings
    float m_gravity[3] = {0, 0, -9.81f}; // Z-up coordinate system
    float m_fixedTimeStep = 1.0f / 60.0f;
    float m_accumulator = 0.0f;
    int m_maxSubSteps = 4;
    
    // Collision tracking
    std::vector<CollisionInfo> m_collisions;
    
    // Physics simulation
    void FixedUpdate(float dt);
    void IntegrateVelocity(float dt);
    void DetectCollisions();
    void ResolveCollisions();
    void IntegratePosition(float dt);
    
    // Collision detection helpers
    bool TestCollision(
        const TransformComponent* transformA, const PhysicsBody* bodyA,
        const TransformComponent* transformB, const PhysicsBody* bodyB,
        CollisionInfo* outInfo
    );
    
    bool RaycastShape(
        std::span<const float, 3> origin, std::span<const float, 3> direction,
        const TransformComponent* transform, const PhysicsBody* body,
        float& outDistance
    );
    
    // Collision response
    void ApplyImpulseInternal(PhysicsBody* body, std::span<const float, 3> impulse);
    void ResolveCollision(
        PhysicsBody* bodyA, TransformComponent* transformA,
        PhysicsBody* bodyB, TransformComponent* transformB,
        const CollisionInfo& collision
    );
};

} // namespace SecretEngine::Physics

// Plugin factory
extern "C" {
    SecretEngine::IPlugin* CreatePhysicsPlugin();
    void DestroyPhysicsPlugin(SecretEngine::IPlugin* plugin);
}
