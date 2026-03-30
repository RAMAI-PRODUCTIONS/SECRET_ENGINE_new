// SecretEngine
// Module: FPSSystems
// Responsibility: FPS gameplay systems (data-oriented, stateless)
// Dependencies: FPSComponents, FPSFastData, Core

#pragma once

#include "FPSComponents.h"
#include "FPSFastData.h"
#include <glm/glm.hpp>
#include <vector>

namespace SecretEngine::FPS {

// Forward declarations
struct IWorld;
struct LinearAllocator;

// ============================================================================
// MOVEMENT SYSTEM (Kinematic Character Controller)
// ============================================================================

class MovementSystem {
public:
    // Process movement for all KCC components
    static void Update(
        IWorld* world,
        float deltaTime,
        Fast::FastPacketQueue<Fast::PlayerMovePacket>& moveQueue,
        Fast::FastPacketQueue<Fast::PlayerLookPacket>& lookQueue
    );
    
private:
    static void ProcessMovementInput(
        uint32_t entityId,
        const Fast::PlayerMovePacket& packet,
        KCCComponent& kcc,
        float deltaTime
    );
    
    static void ProcessLookInput(
        uint32_t entityId,
        const Fast::PlayerLookPacket& packet,
        float deltaTime
    );
    
    static void ApplyGravity(KCCComponent& kcc, float deltaTime);
    static void ResolveCollisions(KCCComponent& kcc, glm::vec3& position);
};

// ============================================================================
// WEAPON SYSTEM
// ============================================================================

class WeaponSystem {
public:
    struct FireResult {
        bool hit;
        uint32_t hitEntity;
        glm::vec3 hitPosition;
        glm::vec3 hitNormal;
        uint8_t hitZone;  // 0=head, 1=torso, 2=limbs
    };
    
    // Update weapon states (reload, fire cooldown)
    static void Update(
        IWorld* world,
        float currentTime,
        Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue,
        Fast::FastPacketQueue<Fast::WeaponFirePacket>& fireEvents
    );
    
    // Process weapon fire (raycast)
    static FireResult ProcessFire(
        IWorld* world,
        uint32_t shooterEntity,
        const glm::vec3& origin,
        const glm::vec3& direction,
        const WeaponComponent& weapon,
        LinearAllocator& frameArena  // For temporary allocations
    );
    
private:
    static void ProcessReload(WeaponComponent& weapon, float currentTime);
    static bool CanFire(const WeaponComponent& weapon, float currentTime);
    static glm::vec3 ApplySpread(const glm::vec3& direction, float spreadAngle);
};

// ============================================================================
// COMBAT SYSTEM
// ============================================================================

class CombatSystem {
public:
    // Process damage events from fast queue
    static void ProcessDamageEvents(
        IWorld* world,
        Fast::FastPacketQueue<Fast::DamageEventPacket>& damageQueue,
        Fast::FastPacketQueue<Fast::KillEventPacket>& killQueue
    );
    
    // Apply damage to entity
    static bool ApplyDamage(
        IWorld* world,
        uint32_t victimEntity,
        uint32_t attackerEntity,
        float damage,
        uint8_t hitZone,
        Fast::FastPacketQueue<Fast::KillEventPacket>& killQueue
    );
    
    // Calculate damage multiplier based on hit zone
    static float GetDamageMultiplier(uint8_t hitZone) {
        switch (hitZone) {
            case 0: return 2.0f;  // Head
            case 1: return 1.0f;  // Torso
            case 2: return 0.75f; // Limbs
            default: return 1.0f;
        }
    }
};

// ============================================================================
// AI SYSTEM
// ============================================================================

class AISystem {
public:
    // Update all bot AI behaviors
    static void Update(
        IWorld* world,
        float deltaTime,
        float currentTime,
        Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue
    );
    
private:
    struct BotDecision {
        AIComponent::Behavior nextBehavior;
        glm::vec3 targetPosition;
        uint32_t targetEntity;
        bool shouldFire;
    };
    
    static BotDecision UpdateBehavior(
        IWorld* world,
        uint32_t botEntity,
        const AIComponent& ai,
        const glm::vec3& botPosition,
        float deltaTime
    );
    
    static uint32_t FindNearestEnemy(
        IWorld* world,
        uint32_t botEntity,
        TeamComponent::Team botTeam,
        const glm::vec3& botPosition,
        float maxRange
    );
    
    static bool HasLineOfSight(
        IWorld* world,
        const glm::vec3& from,
        const glm::vec3& to
    );
};

// ============================================================================
// RESPAWN SYSTEM
// ============================================================================

class RespawnSystem {
public:
    // Update respawn timers and respawn dead entities
    static void Update(
        IWorld* world,
        float deltaTime,
        const SpawnSystemState& spawnState
    );
    
    // Mark entity as dead and start respawn timer
    static void ScheduleRespawn(
        IWorld* world,
        uint32_t entityId,
        TeamComponent::Team team,
        const SpawnSystemState& spawnState
    );
    
private:
    static glm::vec3 GetSpawnPoint(
        const SpawnSystemState& spawnState,
        TeamComponent::Team team
    );
};

// ============================================================================
// SPAWN SYSTEM
// ============================================================================

class SpawnSystem {
public:
    // Initialize spawn points from level data
    static void InitializeSpawnPoints(
        SpawnSystemState& state,
        const std::vector<glm::vec3>& redSpawns,
        const std::vector<glm::vec3>& blueSpawns
    );
    
    // Get next spawn point for team (round-robin)
    static glm::vec3 GetNextSpawn(
        SpawnSystemState& state,
        TeamComponent::Team team
    );
};

// ============================================================================
// MATCH SYSTEM
// ============================================================================

class MatchSystem {
public:
    // Update match state (time, score, phase)
    static void Update(
        IWorld* world,
        MatchState& matchState,
        float deltaTime,
        Fast::FastPacketQueue<Fast::KillEventPacket>& killQueue
    );
    
    // Check if match has ended
    static bool HasMatchEnded(const MatchState& matchState);
    
    // Get winning team
    static TeamComponent::Team GetWinner(const MatchState& matchState);
    
private:
    static void ProcessKillEvents(
        MatchState& matchState,
        Fast::FastPacketQueue<Fast::KillEventPacket>& killQueue
    );
};

// ============================================================================
// PREDICTION SYSTEM (Client-Side Prediction)
// ============================================================================

class PredictionSystem {
public:
    // Save current state snapshot for prediction
    static void SaveSnapshot(
        PredictionComponent& prediction,
        const glm::vec3& position,
        const glm::vec3& velocity,
        float timestamp,
        uint32_t inputSequence
    );
    
    // Reconcile with server state
    static void Reconcile(
        IWorld* world,
        uint32_t entityId,
        PredictionComponent& prediction,
        const glm::vec3& serverPosition,
        const glm::vec3& serverVelocity,
        uint32_t serverInputSequence,
        float currentTime
    );
    
private:
    static void ReplayInputs(
        IWorld* world,
        uint32_t entityId,
        PredictionComponent& prediction,
        uint32_t fromSequence,
        uint32_t toSequence
    );
};

// ============================================================================
// HITBOX SYSTEM (Server-Side Rewind)
// ============================================================================

class HitboxSystem {
public:
    struct HitTestResult {
        bool hit;
        uint8_t hitZone;
        glm::vec3 hitPoint;
    };
    
    // Test ray against hitbox
    static HitTestResult TestRay(
        const HitboxComponent& hitbox,
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        float maxDistance
    );
    
    // Rewind entity to specific time (for lag compensation)
    static void RewindToTime(
        IWorld* world,
        uint32_t entityId,
        float targetTime
    );
    
private:
    static bool RayCapsuleIntersect(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& capsuleStart,
        const glm::vec3& capsuleEnd,
        float capsuleRadius,
        float& outDistance
    );
};

} // namespace SecretEngine::FPS
