// ============================================================================
// SECRET ENGINE - FAST DATA ARCHITECTURE (FDA)
// ============================================================================
// Standardized sub-100ns cross-thread communication protocol.
// Used for: Input, Rendering Commands, Physics Updates, Asset Signaling.
// ============================================================================
#pragma once
#include <atomic>
#include <cstdint>
#include <bit>
#include <new>

namespace SecretEngine::Fast {

#define SE_INLINE [[gnu::always_inline]] inline
#define SE_HOT [[gnu::hot]]
#define SE_UNLIKELY [[unlikely]]
#define SE_RESTRICT __restrict__

// std::hardware_destructive_interference_size is often missing or broken on NDK/Clang
// For ARM64 and x64, 64 bytes is the standard cache line size.
constexpr size_t CACHE_LINE = 64;

enum class PacketType : uint8_t {
    None = 0,
    InputAxis = 1,
    InputAction = 2,
    RenderTransform = 3,
    RenderCommand = 4,
    AssetSignal = 5,
    PhysicsDelta = 6,
    CameraView = 7,
    LightPos = 8,       // lightIdx(metadata) | posX(dataA) | posY(dataB)  — packed 1/32767
    LightPosZColor = 9, // lightIdx(metadata) | posZ_packed(dataA) | colRGB_packed(dataB)
    Cutsom = 255
};

// The standardized 8-byte message format
struct alignas(8) UltraPacket {
    uint32_t packed_state; // [8-bit Type][24-bit Metadata/State]
    int16_t dataA;         // Generic payload A (e.g. X-axis, Transform X)
    int16_t dataB;         // Generic payload B (e.g. Y-axis, Transform Y)
    
    SE_INLINE void Set(PacketType type, uint32_t metadata, int16_t a, int16_t b) noexcept {
        packed_state = (static_cast<uint32_t>(type) << 24) | (metadata & 0x00FFFFFFu);
        dataA = a;
        dataB = b;
    }

    SE_INLINE PacketType GetType() const noexcept {
        return static_cast<PacketType>(packed_state >> 24);
    }

    SE_INLINE uint32_t GetMetadata() const noexcept {
        return packed_state & 0x00FFFFFFu;
    }
};

static_assert(sizeof(UltraPacket) == 8, "FDA Packet must be 8 bytes");

// The standardized Lock-Free Pipe (SPSC)
template<size_t Capacity = 512>
struct alignas(CACHE_LINE) UltraRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    
    alignas(CACHE_LINE) std::atomic<uint32_t> head{0};
    alignas(CACHE_LINE) std::atomic<uint32_t> tail{0};
    alignas(CACHE_LINE) UltraPacket buffer[Capacity];
    
    static constexpr uint32_t MASK = Capacity - 1;
    
    SE_INLINE bool Push(const UltraPacket& packet) noexcept {
        const uint32_t current_head = head.load(std::memory_order_relaxed);
        const uint32_t next_head = (current_head + 1) & MASK;
        const uint32_t current_tail = tail.load(std::memory_order_acquire);
        if (next_head == current_tail) SE_UNLIKELY return false;
        buffer[current_head] = packet;
        head.store(next_head, std::memory_order_release);
        return true;
    }
    
    SE_INLINE bool Pop(UltraPacket& out_packet) noexcept {
        const uint32_t current_tail = tail.load(std::memory_order_relaxed);
        const uint32_t current_head = head.load(std::memory_order_acquire);
        if (current_tail == current_head) SE_UNLIKELY return false;
        out_packet = buffer[current_tail];
        tail.store((current_tail + 1) & MASK, std::memory_order_release);
        return true;
    }

    SE_INLINE uint32_t PopBatch(UltraPacket* SE_RESTRICT out_packets, uint32_t max_count) noexcept {
        const uint32_t current_tail = tail.load(std::memory_order_relaxed);
        const uint32_t current_head = head.load(std::memory_order_acquire);
        if (current_tail == current_head) return 0;
        const uint32_t available = (current_head - current_tail) & MASK;
        const uint32_t to_read = (available < max_count) ? available : max_count;
        for (uint32_t i = 0; i < to_read; ++i) {
            out_packets[i] = buffer[(current_tail + i) & MASK];
        }
        tail.store((current_tail + to_read) & MASK, std::memory_order_release);
        return to_read;
    }
};

} // namespace SecretEngine::Fast
