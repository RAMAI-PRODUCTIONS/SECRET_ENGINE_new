// SecretEngine
// Module: FPSGameLogic
// Responsibility: 8-byte packet definitions
// Dependencies: Core Fast Data

#pragma once
#include <cstdint>
#include <atomic>

namespace SecretEngine::FPS::Fast {

// Movement packet (8 bytes)
struct PlayerMovePacket {
    uint32_t entityId : 24;
    uint32_t sequence : 8;
    int8_t forward;
    int8_t strafe;
    uint8_t flags;
    uint8_t padding;
    
    static constexpr uint8_t FLAG_JUMP = 0x01;
};
static_assert(sizeof(PlayerMovePacket) == 8, "Must be 8 bytes");

// Action packet (8 bytes)
struct PlayerActionPacket {
    uint32_t entityId : 24;
    uint32_t sequence : 8;
    uint8_t action;
    uint8_t padding[3];
    
    static constexpr uint8_t ACTION_FIRE = 0;
    static constexpr uint8_t ACTION_RELOAD = 1;
};
static_assert(sizeof(PlayerActionPacket) == 8, "Must be 8 bytes");

// Damage packet (8 bytes)
struct DamageEventPacket {
    uint32_t victimId : 24;
    uint32_t amount : 8;
    uint32_t attackerId : 24;
    uint32_t padding : 8;
};
static_assert(sizeof(DamageEventPacket) == 8, "Must be 8 bytes");

// Lock-free queue (SPSC)
template<typename PacketT, uint32_t Capacity = 1024>
class FastPacketQueue {
public:
    FastPacketQueue() : m_head(0), m_tail(0) {}
    
    bool Push(const PacketT& packet) {
        uint32_t head = m_head.load(std::memory_order_relaxed);
        uint32_t nextHead = (head + 1) % Capacity;
        uint32_t tail = m_tail.load(std::memory_order_acquire);
        if (nextHead == tail) return false;
        m_packets[head] = packet;
        m_head.store(nextHead, std::memory_order_release);
        return true;
    }
    
    bool Pop(PacketT& packet) {
        uint32_t tail = m_tail.load(std::memory_order_relaxed);
        uint32_t head = m_head.load(std::memory_order_acquire);
        if (tail == head) return false;
        packet = m_packets[tail];
        m_tail.store((tail + 1) % Capacity, std::memory_order_release);
        return true;
    }
    
    bool IsEmpty() const {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }
    
private:
    alignas(64) std::atomic<uint32_t> m_head;
    alignas(64) std::atomic<uint32_t> m_tail;
    PacketT m_packets[Capacity];
};

// Fast data streams
struct FPSFastStreams {
    FastPacketQueue<PlayerMovePacket> moveInput;
    FastPacketQueue<PlayerActionPacket> actionInput;
    FastPacketQueue<DamageEventPacket> damageEvents;
};

} // namespace SecretEngine::FPS::Fast
