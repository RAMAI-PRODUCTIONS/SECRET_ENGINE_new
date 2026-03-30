// SecretEngine
// Module: FPSGameLogic
// Responsibility: FPS game component definitions (POD only)
// Dependencies: Core

#pragma once
#include <cstdint>

namespace SecretEngine::FPS {

// Team assignment
struct TeamComponent {
    enum class Team : uint8_t { None = 0, Red = 1, Blue = 2 };
    Team team = Team::None;
    
    static const uint32_t TypeID = 0x1001;
};

// Health
struct HealthComponent {
    float current = 100.0f;
    float maximum = 100.0f;
    uint32_t lastDamageSource = 0;
    
    static const uint32_t TypeID = 0x1002;
};

// Movement (Kinematic Character Controller)
struct KCCComponent {
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    uint8_t isGrounded : 1;
    uint8_t isJumping : 1;
    uint8_t _padding : 6;
    
    float moveSpeed = 5.0f;
    float jumpForce = 7.0f;
    float gravity = 9.8f;
    float coyoteTime = 0.1f;
    float coyoteTimer = 0.0f;
    
    static const uint32_t TypeID = 0x1003;
};

// Weapon
struct WeaponComponent {
    uint32_t ammoInMag = 30;
    uint32_t ammoReserve = 90;
    uint32_t magSize = 30;
    float fireRate = 0.1f;
    float lastFireTime = 0.0f;
    float damage = 25.0f;
    float range = 100.0f;
    uint8_t isReloading : 1;
    uint8_t _padding : 7;
    
    static const uint32_t TypeID = 0x1004;
};

// AI
struct AIComponent {
    enum class Behavior : uint8_t { Idle, Patrol, Attack };
    Behavior currentBehavior = Behavior::Patrol;
    uint32_t targetEntity = 0;
    float patrolTarget[3] = {0.0f, 0.0f, 0.0f};
    float behaviorTimer = 0.0f;
    float detectionRange = 30.0f;
    float attackRange = 25.0f;
    
    static const uint32_t TypeID = 0x1005;
};

// Match state (global singleton)
struct MatchState {
    uint32_t playerKills = 0;
    uint32_t playerDeaths = 0;
    uint32_t botKills = 0;
    float matchTime = 0.0f;
    uint32_t killLimit = 30;
    bool matchEnded = false;
};

} // namespace SecretEngine::FPS
