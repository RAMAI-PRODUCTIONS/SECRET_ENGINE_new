// SecretEngine
// Module: FPSComponents
// Responsibility: FPS game component definitions (POD only, no behavior)
// Dependencies: Core Entity system

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace SecretEngine::FPS {

// ============================================================================
// COMPONENT TAGS (Zero-size markers)
// ============================================================================

struct PlayerTag {};           // Marks local player entity
struct BotTag {};              // Marks AI-controlled entity
struct BulletTag {};           // Marks bullet entity
struct DeadTag {};             // Marks entity awaiting respawn

// ============================================================================
// CORE COMPONENTS (Pure Data - No Methods)
// ============================================================================

// Team assignment
struct TeamComponent {
    enum class Team : uint8_t {
        None = 0,
        Red = 1,
        Blue = 2
    };
    Team team = Team::None;
};

// Health and damage state
struct HealthComponent {
    float current = 100.0f;
    float maximum = 100.0f;
    uint32_t lastDamageSource = 0;  // Entity handle that dealt damage
    float lastDamageTime = 0.0f;
};

// Kinematic Character Controller state
struct KCCComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 groundNormal{0.0f, 1.0f, 0.0f};
    
    uint8_t isGrounded : 1;
    uint8_t isCrouching : 1;
    uint8_t isSprinting : 1;
    uint8_t isAiming : 1;
    uint8_t _padding : 4;
    
    float moveSpeed = 5.0f;
    float sprintMultiplier = 1.5f;
    float crouchMultiplier = 0.5f;
};

// Weapon state (per entity)
struct WeaponComponent {
    enum class WeaponType : uint8_t {
        Rifle = 0,
        Shotgun = 1,
        Sniper = 2,
        Pistol = 3
    };
    
    WeaponType type = WeaponType::Rifle;
    uint32_t ammoInMag = 30;
    uint32_t ammoReserve = 90;
    uint32_t magSize = 30;
    
    float fireRate = 0.1f;          // Seconds between shots
    float reloadTime = 2.0f;
    float range = 100.0f;
    float damage = 25.0f;
    float spread = 0.01f;           // Cone of fire
    
    float lastFireTime = 0.0f;
    float reloadStartTime = -1.0f;  // -1 = not reloading
    
    uint8_t isReloading : 1;
    uint8_t isFiring : 1;
    uint8_t _padding : 6;
};

// Bot AI state machine
struct AIComponent {
    enum class Behavior : uint8_t {
        Idle = 0,
        Patrol = 1,
        Chase = 2,
        Attack = 3,
        TakeCover = 4,
        Retreat = 5
    };
    
    Behavior currentBehavior = Behavior::Patrol;
    uint32_t targetEntity = 0;      // Entity handle of current target
    glm::vec3 targetPosition{0.0f}; // Waypoint or last known enemy position
    
    float detectionRange = 50.0f;
    float attackRange = 40.0f;
    float accuracy = 0.8f;          // 0-1 hit probability
    
    float behaviorTimer = 0.0f;     // Generic timer for state duration
    float lastSeenTargetTime = 0.0f;
};

// Respawn state
struct RespawnComponent {
    float respawnTimer = 0.0f;
    glm::vec3 spawnPoint{0.0f};
    TeamComponent::Team spawnTeam = TeamComponent::Team::None;
};

// Match statistics (per player)
struct StatsComponent {
    uint32_t kills = 0;
    uint32_t deaths = 0;
    uint32_t assists = 0;
    uint32_t shotsFired = 0;
    uint32_t shotsHit = 0;
    uint32_t headshots = 0;
};

// Client-side prediction state (for networking)
struct PredictionComponent {
    uint32_t lastAckedInputSequence = 0;
    uint32_t currentInputSequence = 0;
    
    // Ring buffer of recent states for reconciliation
    struct StateSnapshot {
        glm::vec3 position;
        glm::vec3 velocity;
        float timestamp;
        uint32_t inputSequence;
    };
    
    static constexpr uint32_t HISTORY_SIZE = 32;
    StateSnapshot history[HISTORY_SIZE];
    uint32_t historyHead = 0;
};

// Hitbox component (for server-side rewind)
struct HitboxComponent {
    enum class HitZone : uint8_t {
        Head = 0,
        Torso = 1,
        Limbs = 2
    };
    
    struct Capsule {
        glm::vec3 start;
        glm::vec3 end;
        float radius;
        HitZone zone;
    };
    
    Capsule capsules[3];  // Head, torso, legs
    uint32_t numCapsules = 3;
};

// ============================================================================
// GAME STATE COMPONENTS (Global/Singleton)
// ============================================================================

struct MatchState {
    enum class Phase : uint8_t {
        Warmup = 0,
        Active = 1,
        Overtime = 2,
        Ended = 3
    };
    
    Phase currentPhase = Phase::Warmup;
    uint32_t redTeamScore = 0;
    uint32_t blueTeamScore = 0;
    uint32_t scoreLimit = 30;
    
    float matchTimeRemaining = 300.0f;  // 5 minutes
    float phaseStartTime = 0.0f;
};

// Spawn system state
struct SpawnSystemState {
    static constexpr uint32_t MAX_SPAWN_POINTS = 16;
    
    glm::vec3 redSpawnPoints[MAX_SPAWN_POINTS];
    glm::vec3 blueSpawnPoints[MAX_SPAWN_POINTS];
    uint32_t numRedSpawns = 0;
    uint32_t numBlueSpawns = 0;
    
    uint32_t nextRedSpawnIndex = 0;
    uint32_t nextBlueSpawnIndex = 0;
};

} // namespace SecretEngine::FPS
