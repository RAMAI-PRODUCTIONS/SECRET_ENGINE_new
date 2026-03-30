// SecretEngine
// Module: FPSFastData
// Responsibility: 8-byte packet definitions for FPS game events
// Dependencies: Core Fast Data Architecture

#pragma once

#include <cstdint>

namespace SecretEngine::FPS::Fast {

// ============================================================================
// 8-BYTE PACKET TYPES (Sub-100ns Communication)
// ============================================================================

// Packet type IDs (extend core FastData enum)
enum class PacketType : uint8_t {
    // Movement packets (Input → Game Logic)
    PlayerMove      = 32,  // Movement input (forward, strafe, jump)
    PlayerLook      = 33,  // Look direction (yaw, pitch deltas)
    PlayerAction    = 34,  // Fire, reload, crouch, sprint
    
    // Combat packets (Game Logic → Game Logic)
    DamageEvent     = 40,  // Damage dealt to entity
    KillEvent       = 41,  // Entity killed notification
    WeaponFire      = 42,  // Weapon fired event
    
    // Render packets (Game Logic → Renderer)
    MuzzleFlash     = 50,  // Spawn muzzle flash effect
    BulletTrace     = 51,  // Visual bullet tracer
    HitMarker       = 52,  // Hit confirmation visual
    
    // Network packets (Game Logic → Network)
    InputCommand    = 60,  // Client input to send
    StateUpdate     = 61,  // Server state received
};

// ============================================================================
// PACKET STRUCTURES (Each exactly 8 bytes)
// ============================================================================

// Movement input packet
struct PlayerMovePacket {
    uint32_t entityId : 24;     // Entity handle
    uint32_t sequence : 8;      // Input sequence number
    
    int8_t forward;             // -127 to 127
    int8_t strafe;              // -127 to 127
    uint8_t flags;              // jump, crouch, sprint bits
    uint8_t padding;
    
    static constexpr uint8_t FLAG_JUMP = 0x01;
    static constexpr uint8_t FLAG_CROUCH = 0x02;
    static constexpr uint8_t FLAG_SPRINT = 0x04;
};
static_assert(sizeof(PlayerMovePacket) == 8, "Must be 8 bytes");

// Look direction packet (delta-based)
struct PlayerLookPacket {
    uint32_t entityId : 24;
    uint32_t sequence : 8;
    
    int16_t yawDelta;           // Rotation delta in 1/100th degrees
    int16_t pitchDelta;         // Rotation delta in 1/100th degrees
};
static_assert(sizeof(PlayerLookPacket) == 8, "Must be 8 bytes");

// Action packet (fire, reload, etc.)
struct PlayerActionPacket {
    uint32_t entityId : 24;
    uint32_t sequence : 8;
    
    uint8_t action;             // 0=fire, 1=reload, 2=switch weapon, etc.
    uint8_t weaponSlot;
    uint16_t padding;
    
    static constexpr uint8_t ACTION_FIRE = 0;
    static constexpr uint8_t ACTION_RELOAD = 1;
    static constexpr uint8_t ACTION_SWITCH = 2;
};
static_assert(sizeof(PlayerActionPacket) == 8, "Must be 8 bytes");

// Damage event packet
struct DamageEventPacket {
    uint32_t victimId : 24;     // Entity that took damage
    uint32_t amount : 8;        // Damage amount (0-255)
    
    uint32_t attackerId : 24;   // Entity that dealt damage
    uint32_t hitZone : 2;       // 0=head, 1=torso, 2=limbs
    uint32_t weaponType : 3;    // Weapon that dealt damage
    uint32_t padding : 3;
};
static_assert(sizeof(DamageEventPacket) == 8, "Must be 8 bytes");

// Kill event packet
struct KillEventPacket {
    uint32_t victimId : 24;
    uint32_t killerTeam : 2;    // 0=none, 1=red, 2=blue
    uint32_t victimTeam : 2;
    uint32_t padding : 4;
    
    uint32_t killerId : 24;
    uint32_t killType : 4;      // 0=normal, 1=headshot, 2=melee, etc.
    uint32_t padding2 : 4;
};
static_assert(sizeof(KillEventPacket) == 8, "Must be 8 bytes");

// Weapon fire packet
struct WeaponFirePacket {
    uint32_t entityId : 24;     // Shooter entity
    uint32_t weaponType : 4;    // Weapon that fired
    uint32_t padding : 4;
    
    uint16_t directionX;        // Normalized direction * 32767
    uint16_t directionY;
};
static_assert(sizeof(WeaponFirePacket) == 8, "Must be 8 bytes");

// Muzzle flash visual effect
struct MuzzleFlashPacket {
    uint32_t entityId : 24;     // Entity firing
    uint32_t weaponType : 4;
    uint32_t padding : 4;
    
    uint16_t positionX;         // Position offset * 100
    uint16_t positionY;
};
static_assert(sizeof(MuzzleFlashPacket) == 8, "Must be 8 bytes");

// Bullet trace visual
struct BulletTracePacket {
    uint16_t startX;            // World position * 10
    uint16_t startY;
    uint16_t endX;
    uint16_t endY;
};
static_assert(sizeof(BulletTracePacket) == 8, "Must be 8 bytes");

// Hit marker visual feedback
struct HitMarkerPacket {
    uint32_t timestamp;         // Frame timestamp
    
    uint8_t hitType;            // 0=normal, 1=headshot, 2=kill
    uint8_t hitZone;
    uint16_t damage;            // Damage dealt
};
static_assert(sizeof(HitMarkerPacket) == 8, "Must be 8 bytes");

// ============================================================================
// PACKET QUEUE (Lock-Free SPSC)
// ============================================================================

// Simple ring buffer for Fast Data packets
template<typename PacketT, uint32_t Capacity = 1024>
class FastPacketQueue {
public:
    FastPacketQueue() : m_head(0), m_tail(0) {}
    
    // Producer: Push packet (returns false if full)
    bool Push(const PacketT& packet) {
        uint32_t head = m_head.load(std::memory_order_relaxed);
        uint32_t nextHead = (head + 1) % Capacity;
        
        uint32_t tail = m_tail.load(std::memory_order_acquire);
        if (nextHead == tail) {
            return false;  // Queue full
        }
        
        m_packets[head] = packet;
        m_head.store(nextHead, std::memory_order_release);
        return true;
    }
    
    // Consumer: Pop packet (returns false if empty)
    bool Pop(PacketT& packet) {
        uint32_t tail = m_tail.load(std::memory_order_relaxed);
        uint32_t head = m_head.load(std::memory_order_acquire);
        
        if (tail == head) {
            return false;  // Queue empty
        }
        
        packet = m_packets[tail];
        m_tail.store((tail + 1) % Capacity, std::memory_order_release);
        return true;
    }
    
    // Check if empty
    bool IsEmpty() const {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
private:
    alignas(64) std::atomic<uint32_t> m_head;
    alignas(64) std::atomic<uint32_t> m_tail;
    PacketT m_packets[Capacity];
};

// ============================================================================
// FAST DATA STREAMS (Plugin Communication)
// ============================================================================

struct FPSFastStreams {
    // Input → Game Logic
    FastPacketQueue<PlayerMovePacket> moveInput;
    FastPacketQueue<PlayerLookPacket> lookInput;
    FastPacketQueue<PlayerActionPacket> actionInput;
    
    // Game Logic → Game Logic (events)
    FastPacketQueue<DamageEventPacket> damageEvents;
    FastPacketQueue<KillEventPacket> killEvents;
    FastPacketQueue<WeaponFirePacket> weaponEvents;
    
    // Game Logic → Renderer
    FastPacketQueue<MuzzleFlashPacket> muzzleFlashes;
    FastPacketQueue<BulletTracePacket> bulletTraces;
    FastPacketQueue<HitMarkerPacket> hitMarkers;
};

} // namespace SecretEngine::FPS::Fast
