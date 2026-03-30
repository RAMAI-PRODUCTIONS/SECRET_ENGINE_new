// SecretEngine
// Module: FPSSystems
// Responsibility: FPS gameplay systems implementation
// Dependencies: Core, PhysicsPlugin

#include "FPSGamePlugin.h"
#include "../../PhysicsPlugin/src/PhysicsPlugin.h"
#include <random>
#include <cmath>

namespace SecretEngine::FPS {

// Vector math helpers
namespace {
    inline float Dot(const float a[3], const float b[3]) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    }
    
    inline float Length(const float v[3]) {
        return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    }
    
    inline void Normalize(const float v[3], float out[3]) {
        float len = Length(v);
        if (len > 0.0001f) {
            out[0] = v[0] / len;
            out[1] = v[1] / len;
            out[2] = v[2] / len;
        } else {
            out[0] = out[1] = out[2] = 0.0f;
        }
    }
    
    inline void Sub(const float a[3], const float b[3], float out[3]) {
        out[0] = a[0] - b[0];
        out[1] = a[1] - b[1];
        out[2] = a[2] - b[2];
    }
    
    inline void Add(const float a[3], const float b[3], float out[3]) {
        out[0] = a[0] + b[0];
        out[1] = a[1] + b[1];
        out[2] = a[2] + b[2];
    }
    
    inline void Scale(const float v[3], float s, float out[3]) {
        out[0] = v[0] * s;
        out[1] = v[1] * s;
        out[2] = v[2] * s;
    }
    
    inline void ScaleAdd(const float a[3], const float b[3], float s, float out[3]) {
        out[0] = a[0] + b[0] * s;
        out[1] = a[1] + b[1] * s;
        out[2] = a[2] + b[2] * s;
    }
    
    inline void Cross(const float a[3], const float b[3], float out[3]) {
        out[0] = a[1]*b[2] - a[2]*b[1];
        out[1] = a[2]*b[0] - a[0]*b[2];
        out[2] = a[0]*b[1] - a[1]*b[0];
    }
    
    inline void Copy(const float src[3], float dst[3]) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    
    inline void Set(float x, float y, float z, float out[3]) {
        out[0] = x;
        out[1] = y;
        out[2] = z;
    }
}

// ============================================================================
// MOVEMENT SYSTEM
// ============================================================================
namespace MovementSystem {
    void Update(
        IWorld* world,
        SecretEngine::Physics::PhysicsPlugin* physics,
        float deltaTime,
        Fast::FastPacketQueue<Fast::PlayerMovePacket>& moveQueue
    ) {
        // Process movement input packets
        Fast::PlayerMovePacket packet;
        while (moveQueue.Pop(packet)) {
            Entity entity = {packet.entityId, 1};
            
            auto* kcc = static_cast<KCCComponent*>(
                world->GetComponent(entity, KCCComponent::TypeID)
            );
            if (!kcc) continue;
            
            auto* transform = static_cast<TransformComponent*>(
                world->GetComponent(entity, TransformComponent::TypeID)
            );
            if (!transform) continue;
            
            // Ground check
            bool wasGrounded = kcc->isGrounded;
            kcc->isGrounded = physics->CheckGround(transform->position, 0.4f, 0.2f);
            
            // Update coyote time
            if (wasGrounded && !kcc->isGrounded) {
                kcc->coyoteTimer = kcc->coyoteTime;
            }
            if (kcc->coyoteTimer > 0.0f) {
                kcc->coyoteTimer -= deltaTime;
            }
            
            // Process movement input
            float forwardInput = packet.forward / 127.0f;
            float strafeInput = packet.strafe / 127.0f;
            
            float speed = kcc->moveSpeed;
            if (!kcc->isGrounded) {
                speed *= 0.3f; // Air control
            }
            
            // Calculate movement direction (camera-relative)
            // Get forward from transform rotation (simplified - assumes rotation[1] is yaw)
            float yaw = transform->rotation[1];
            float forward[3] = {std::sin(yaw), std::cos(yaw), 0.0f}; // Z-up: XY plane is horizontal
            float right[3];
            float up[3] = {0, 0, 1}; // Z-up coordinate system
            Cross(forward, up, right);
            
            // Set horizontal velocity (XY plane in Z-up)
            kcc->velocity[0] = (forward[0] * forwardInput + right[0] * strafeInput) * speed;
            kcc->velocity[1] = (forward[1] * forwardInput + right[1] * strafeInput) * speed;
            
            // Handle jump (Z-up: Z is vertical)
            if (packet.flags & Fast::PlayerMovePacket::FLAG_JUMP) {
                bool canJump = kcc->isGrounded || kcc->coyoteTimer > 0.0f;
                if (canJump && !kcc->isJumping) {
                    kcc->velocity[2] = kcc->jumpForce; // Z is up
                    kcc->isJumping = true;
                    kcc->isGrounded = false;
                    kcc->coyoteTimer = 0.0f;
                }
            }
            
            // Apply gravity (Z-up)
            if (!kcc->isGrounded) {
                kcc->velocity[2] -= kcc->gravity * deltaTime; // Z is vertical
                if (kcc->velocity[2] < -20.0f) {
                    kcc->velocity[2] = -20.0f; // Terminal velocity
                }
            } else {
                if (kcc->velocity[2] < 0.0f) {
                    kcc->velocity[2] = 0.0f;
                }
                kcc->isJumping = false;
            }
            
            // Apply velocity to position
            ScaleAdd(transform->position, kcc->velocity, deltaTime, transform->position);
            
            // Simple ground clamp (Z-up)
            if (transform->position[2] < 0.1f) {
                transform->position[2] = 0.1f; // Z is vertical
                kcc->velocity[2] = 0.0f;
                kcc->isGrounded = true;
            }
        }
    }
}

// ============================================================================
// WEAPON SYSTEM
// ============================================================================
namespace WeaponSystem {
    void Update(
        IWorld* world,
        SecretEngine::Physics::PhysicsPlugin* physics,
        float currentTime,
        Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue,
        Fast::FastPacketQueue<Fast::DamageEventPacket>& damageQueue,
        ILogger* logger
    ) {
        // Process action packets
        Fast::PlayerActionPacket packet;
        while (actionQueue.Pop(packet)) {
            Entity entity = {packet.entityId, 1};
            
            auto* weapon = static_cast<WeaponComponent*>(
                world->GetComponent(entity, WeaponComponent::TypeID)
            );
            if (!weapon) continue;
            
            if (packet.action == Fast::PlayerActionPacket::ACTION_FIRE) {
                // DEBUG: Log fire attempt
                char debugMsg[256];
                snprintf(debugMsg, sizeof(debugMsg), "🔫 FIRE ACTION received for entity %u", packet.entityId);
                if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                
                // Check if can fire
                if (weapon->isReloading) {
                    snprintf(debugMsg, sizeof(debugMsg), "❌ Cannot fire - weapon reloading (entity %u)", packet.entityId);
                    if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                    continue;
                }
                if (weapon->ammoInMag == 0) {
                    snprintf(debugMsg, sizeof(debugMsg), "❌ Cannot fire - no ammo (entity %u)", packet.entityId);
                    if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                    continue;
                }
                
                float timeSinceLastFire = currentTime - weapon->lastFireTime;
                if (timeSinceLastFire < weapon->fireRate) {
                    snprintf(debugMsg, sizeof(debugMsg), "❌ Cannot fire - fire rate cooldown %.3fs (entity %u)", 
                             weapon->fireRate - timeSinceLastFire, packet.entityId);
                    if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                    continue;
                }
                
                // Fire weapon
                weapon->lastFireTime = currentTime;
                weapon->ammoInMag--;
                
                snprintf(debugMsg, sizeof(debugMsg), "✅ FIRING! Ammo: %u/%u (entity %u)", 
                         weapon->ammoInMag, weapon->magSize, packet.entityId);
                if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                
                // Raycast from entity position
                auto* transform = static_cast<TransformComponent*>(
                    world->GetComponent(entity, TransformComponent::TypeID)
                );
                if (!transform) {
                    if (logger) logger->LogInfo("WeaponSystem", "❌ No transform component found!");
                    continue;
                }
                
                if (logger) logger->LogInfo("WeaponSystem", "✅ Transform found, preparing raycast...");
                
                float origin[3];
                Set(transform->position[0], transform->position[1], transform->position[2] + 0.5f, origin); // Z-up: raise Z for eye height
                
                // Get direction from rotation (simplified - assumes rotation[1] is yaw)
                float yaw = transform->rotation[1];
                float direction[3] = {std::sin(yaw), std::cos(yaw), 0.0f}; // Z-up: direction in XY plane
                
                snprintf(debugMsg, sizeof(debugMsg), 
                         "🎯 RAYCAST: origin=(%.2f, %.2f, %.2f) dir=(%.2f, %.2f, %.2f) range=%.1f", 
                         origin[0], origin[1], origin[2], 
                         direction[0], direction[1], direction[2], 
                         weapon->range);
                if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                
                auto hit = physics->Raycast(origin, direction, weapon->range);
                
                if (hit.hit) {
                    if (hit.entityId != packet.entityId) {
                        snprintf(debugMsg, sizeof(debugMsg), 
                                 "💥 HIT! Entity %u hit entity %u at distance %.2f - dealing %.0f damage", 
                                 packet.entityId, hit.entityId, hit.distance, weapon->damage);
                        if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                        
                        // Create damage event
                        Fast::DamageEventPacket dmgPacket;
                        dmgPacket.victimId = hit.entityId;
                        dmgPacket.amount = static_cast<uint32_t>(weapon->damage);
                        dmgPacket.attackerId = packet.entityId;
                        damageQueue.Push(dmgPacket);
                    } else {
                        snprintf(debugMsg, sizeof(debugMsg), "⚠️ Raycast hit self (entity %u) - ignoring", packet.entityId);
                        if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                    }
                } else {
                    snprintf(debugMsg, sizeof(debugMsg), "❌ MISS - raycast hit nothing (entity %u)", packet.entityId);
                    if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                }
            }
            else if (packet.action == Fast::PlayerActionPacket::ACTION_RELOAD) {
                if (!weapon->isReloading && weapon->ammoInMag < weapon->magSize) {
                    weapon->isReloading = true;
                    char debugMsg[128];
                    snprintf(debugMsg, sizeof(debugMsg), "🔄 RELOADING weapon (entity %u)", packet.entityId);
                    if (logger) logger->LogInfo("WeaponSystem", debugMsg);
                }
            }
        }
        
        // Update reloads
        auto& weaponEntities = world->GetAllEntities();
        for (const Entity& entity : weaponEntities) {
            auto* weapon = static_cast<WeaponComponent*>(
                world->GetComponent(entity, WeaponComponent::TypeID)
            );
            if (!weapon) continue;
            
            if (weapon->isReloading) {
                // Simple instant reload for now
                uint32_t needed = weapon->magSize - weapon->ammoInMag;
                uint32_t toReload = (needed < weapon->ammoReserve) ? needed : weapon->ammoReserve;
                weapon->ammoInMag += toReload;
                weapon->ammoReserve -= toReload;
                weapon->isReloading = false;
            }
        }
    }
}

// ============================================================================
// COMBAT SYSTEM
// ============================================================================
namespace CombatSystem {
    void ProcessDamageEvents(
        IWorld* world,
        Fast::FastPacketQueue<Fast::DamageEventPacket>& damageQueue,
        MatchState& matchState
    ) {
        Fast::DamageEventPacket packet;
        while (damageQueue.Pop(packet)) {
            Entity victim = {packet.victimId, 1};
            
            auto* health = static_cast<HealthComponent*>(
                world->GetComponent(victim, HealthComponent::TypeID)
            );
            if (!health) continue;
            
            health->current -= packet.amount;
            health->lastDamageSource = packet.attackerId;
            
            // Check for kill
            if (health->current <= 0.0f) {
                health->current = 0.0f;
                
                // Update match stats
                Entity attacker = {packet.attackerId, 1};
                bool victimIsPlayer = world->GetComponent(victim, AIComponent::TypeID) == nullptr;
                bool attackerIsPlayer = world->GetComponent(attacker, AIComponent::TypeID) == nullptr;
                
                if (attackerIsPlayer) {
                    matchState.playerKills++;
                } else {
                    matchState.botKills++;
                }
                
                if (victimIsPlayer) {
                    matchState.playerDeaths++;
                }
                
                // Respawn (Z-up)
                health->current = health->maximum;
                auto* transform = static_cast<TransformComponent*>(
                    world->GetComponent(victim, TransformComponent::TypeID)
                );
                if (transform) {
                    Set(0, 0, 1, transform->position); // Z-up: spawn at Z=1
                }
            }
        }
    }
}

// ============================================================================
// AI SYSTEM
// ============================================================================
namespace AISystem {
    void Update(
        IWorld* world,
        float deltaTime,
        Fast::FastPacketQueue<Fast::PlayerActionPacket>& actionQueue,
        Entity playerEntity
    ) {
        auto& entities = world->GetAllEntities();
        
        for (const Entity& botEntity : entities) {
            auto* ai = static_cast<AIComponent*>(
                world->GetComponent(botEntity, AIComponent::TypeID)
            );
            if (!ai) continue;
            
            auto* transform = static_cast<TransformComponent*>(
                world->GetComponent(botEntity, TransformComponent::TypeID)
            );
            if (!transform) continue;
            
            auto* kcc = static_cast<KCCComponent*>(
                world->GetComponent(botEntity, KCCComponent::TypeID)
            );
            if (!kcc) continue;
            
            ai->behaviorTimer += deltaTime;
            
            // Get player position
            float playerPos[3] = {0, 0, 0};
            auto* playerTransform = static_cast<TransformComponent*>(
                world->GetComponent(playerEntity, TransformComponent::TypeID)
            );
            if (playerTransform) {
                Copy(playerTransform->position, playerPos);
            }
            
            float toPlayer[3];
            Sub(playerPos, transform->position, toPlayer);
            float distToPlayer = Length(toPlayer);
            
            // Simple AI state machine
            if (distToPlayer < ai->detectionRange) {
                ai->currentBehavior = AIComponent::Behavior::Attack;
                ai->targetEntity = playerEntity.id;
            } else {
                ai->currentBehavior = AIComponent::Behavior::Patrol;
            }
            
            switch (ai->currentBehavior) {
                case AIComponent::Behavior::Patrol: {
                    // Move to patrol target (Z-up)
                    float toTarget[3];
                    Sub(ai->patrolTarget, transform->position, toTarget);
                    toTarget[2] = 0; // Z-up: zero out vertical component
                    
                    float distToTarget = Length(toTarget);
                    if (distToTarget < 1.0f || ai->behaviorTimer > 3.0f) {
                        // Pick new patrol target (Z-up: XY plane)
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_real_distribution<float> dist(-15.0f, 15.0f);
                        Set(dist(gen), dist(gen), 0, ai->patrolTarget); // Z-up: patrol on XY plane
                        ai->behaviorTimer = 0.0f;
                    }
                    
                    if (distToTarget > 0.1f) {
                        float moveDir[3];
                        Normalize(toTarget, moveDir);
                        kcc->velocity[0] = moveDir[0] * kcc->moveSpeed * 0.5f;
                        kcc->velocity[1] = moveDir[1] * kcc->moveSpeed * 0.5f; // Z-up: Y is horizontal
                        
                        // Update rotation to face movement direction (Z-up: XY plane)
                        transform->rotation[1] = std::atan2(moveDir[1], moveDir[0]);
                    }
                    break;
                }
                
                case AIComponent::Behavior::Attack: {
                    // Face player (Z-up)
                    float toPlayerFlat[3];
                    Copy(toPlayer, toPlayerFlat);
                    toPlayerFlat[2] = 0; // Z-up: zero out vertical component
                    float distFlat = Length(toPlayerFlat);
                    
                    if (distFlat > 0.1f) {
                        Normalize(toPlayerFlat, toPlayerFlat);
                        transform->rotation[1] = std::atan2(toPlayerFlat[1], toPlayerFlat[0]); // Z-up: XY plane rotation
                    }
                    
                    // Move toward player if too far (Z-up)
                    if (distToPlayer > ai->attackRange * 0.7f) {
                        float moveDir[3];
                        Normalize(toPlayerFlat, moveDir);
                        kcc->velocity[0] = moveDir[0] * kcc->moveSpeed;
                        kcc->velocity[1] = moveDir[1] * kcc->moveSpeed; // Z-up: Y is horizontal
                    } else {
                        kcc->velocity[0] = 0;
                        kcc->velocity[1] = 0;
                    }
                    
                    // Shoot at player
                    if (distToPlayer < ai->attackRange && ai->behaviorTimer > 0.5f) {
                        Fast::PlayerActionPacket firePacket;
                        firePacket.entityId = botEntity.id;
                        firePacket.action = Fast::PlayerActionPacket::ACTION_FIRE;
                        actionQueue.Push(firePacket);
                        ai->behaviorTimer = 0.0f;
                    }
                    break;
                }
                
                default:
                    break;
            }
            
            // Apply bot movement
            ScaleAdd(transform->position, kcc->velocity, deltaTime, transform->position);
            // Z-up: clamp vertical position
            if (transform->position[2] < 0.1f) {
                transform->position[2] = 0.1f;
            }
        }
    }
}

} // namespace SecretEngine::FPS
