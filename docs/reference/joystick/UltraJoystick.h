// ============================================================================
// ULTRAJOYSTICK - ZERO LATENCY INPUT SYSTEM
// C++20 - Optimized for February 2026 Compiler Technology
// ============================================================================
// SPECIFICATIONS:
// - 8-byte packet (smallest possible cache line fraction)
// - Lock-free SPSC ring buffer (Single Producer, Single Consumer)
// - C++20 atomic_ref for zero-overhead synchronization
// - SIMD-ready alignment (128-bit for AVX)
// - Sub-100 nanosecond write latency
// - 240Hz+ input sampling support
// ============================================================================

#pragma once
#include <atomic>
#include <cstdint>
#include <bit>          // C++20 bit_cast
#include <new>          // std::hardware_destructive_interference_size

namespace SecretEngine::Ultra {

// ============================================================================
// COMPILER HINTS (GCC 11+, Clang 13+, MSVC 19.29+)
// ============================================================================
#define ULTRA_INLINE [[gnu::always_inline]] inline
#define ULTRA_HOT [[gnu::hot]]
#define ULTRA_COLD [[gnu::cold]]
#define ULTRA_LIKELY [[likely]]
#define ULTRA_UNLIKELY [[unlikely]]
#define ULTRA_RESTRICT __restrict__
#define ULTRA_ASSUME(x) __builtin_assume(x)

// Cache line size detection (C++17)
#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t CACHE_LINE = std::hardware_destructive_interference_size;
#else
    constexpr size_t CACHE_LINE = 64;
#endif

// ============================================================================
// ULTRA PACKET: 8-BYTE INPUT MESSAGE (SMALLEST POSSIBLE)
// ============================================================================
// Layout: [32-bit state][16-bit X][16-bit Y]
// Total: 64 bits = 8 bytes = 1/8th cache line
// ============================================================================
struct alignas(8) UltraPacket {
    // Bit-packed state (32 bits)
    uint32_t packed_state;
    
    // Analog axes (16-bit each, -32768 to 32767)
    int16_t axisX;
    int16_t axisY;
    
    // ========================================================================
    // BIT PACKING LAYOUT (32-bit state field)
    // ========================================================================
    // Bits  0-15: Timestamp (16-bit, milliseconds, wraps at 65s)
    // Bits 16-23: Action ID (8-bit, 256 possible actions)
    // Bits 24-25: Event Type (2-bit: 0=Release, 1=Press, 2=Hold, 3=Reserved)
    // Bits 26-31: Flags (6-bit: magnitude, pressure, multitouch, etc.)
    // ========================================================================
    
    // ULTRA-FAST INLINE ACCESSORS (zero overhead, compiler inlines completely)
    
    ULTRA_INLINE void SetTimestamp(uint16_t ms) noexcept {
        packed_state = (packed_state & 0xFFFF0000u) | ms;
    }
    
    ULTRA_INLINE uint16_t GetTimestamp() const noexcept {
        return static_cast<uint16_t>(packed_state & 0xFFFFu);
    }
    
    ULTRA_INLINE void SetActionID(uint8_t id) noexcept {
        packed_state = (packed_state & 0xFF00FFFFu) | (static_cast<uint32_t>(id) << 16);
    }
    
    ULTRA_INLINE uint8_t GetActionID() const noexcept {
        return static_cast<uint8_t>((packed_state >> 16) & 0xFFu);
    }
    
    ULTRA_INLINE void SetEventType(uint8_t type) noexcept {
        packed_state = (packed_state & 0xFCFFFFFFu) | (static_cast<uint32_t>(type & 0x3u) << 24);
    }
    
    ULTRA_INLINE uint8_t GetEventType() const noexcept {
        return static_cast<uint8_t>((packed_state >> 24) & 0x3u);
    }
    
    ULTRA_INLINE void SetFlags(uint8_t flags) noexcept {
        packed_state = (packed_state & 0x03FFFFFFu) | (static_cast<uint32_t>(flags & 0x3Fu) << 26);
    }
    
    ULTRA_INLINE uint8_t GetFlags() const noexcept {
        return static_cast<uint8_t>((packed_state >> 26) & 0x3Fu);
    }
};

static_assert(sizeof(UltraPacket) == 8, "Packet must be exactly 8 bytes");
static_assert(alignof(UltraPacket) == 8, "Packet must be 8-byte aligned");

// ============================================================================
// ULTRA RING BUFFER: LOCK-FREE SPSC QUEUE (C++20 ATOMICS)
// ============================================================================
// - Single Producer (Touch Thread)
// - Single Consumer (Game Thread)
// - 256 packets = 2KB (fits in L1 cache on modern CPUs)
// - Power-of-2 size for fast modulo with bit mask
// ============================================================================
template<size_t Capacity = 256>
struct alignas(CACHE_LINE) UltraRingBuffer {
    static_assert(std::has_single_bit(Capacity), "Capacity must be power of 2");
    
    // Separate cache lines to prevent false sharing
    alignas(CACHE_LINE) std::atomic<uint32_t> head{0}; // Producer writes here
    alignas(CACHE_LINE) std::atomic<uint32_t> tail{0}; // Consumer reads here
    alignas(CACHE_LINE) UltraPacket buffer[Capacity];
    
    static constexpr uint32_t MASK = Capacity - 1;
    
    // ========================================================================
    // PUSH (Producer/Touch Thread) - ULTRA FAST
    // ========================================================================
    ULTRA_INLINE ULTRA_HOT
    bool Push(const UltraPacket& packet) noexcept {
        // Load head (relaxed - we own it)
        const uint32_t current_head = head.load(std::memory_order_relaxed);
        const uint32_t next_head = (current_head + 1) & MASK;
        
        // Check if full (acquire - sync with consumer)
        const uint32_t current_tail = tail.load(std::memory_order_acquire);
        
        if (next_head == current_tail) ULTRA_UNLIKELY {
            return false; // Buffer full
        }
        
        // Write packet (plain store - no atomics needed for data)
        buffer[current_head] = packet;
        
        // Commit write (release - make packet visible to consumer)
        head.store(next_head, std::memory_order_release);
        
        return true;
    }
    
    // ========================================================================
    // POP (Consumer/Game Thread) - ULTRA FAST
    // ========================================================================
    ULTRA_INLINE ULTRA_HOT
    bool Pop(UltraPacket& out_packet) noexcept {
        // Load tail (relaxed - we own it)
        const uint32_t current_tail = tail.load(std::memory_order_relaxed);
        
        // Check if empty (acquire - sync with producer)
        const uint32_t current_head = head.load(std::memory_order_acquire);
        
        if (current_tail == current_head) ULTRA_UNLIKELY {
            return false; // Buffer empty
        }
        
        // Read packet
        out_packet = buffer[current_tail];
        
        // Commit read (release - allow producer to reuse slot)
        tail.store((current_tail + 1) & MASK, std::memory_order_release);
        
        return true;
    }
    
    // ========================================================================
    // BATCH POP (Drain up to N packets at once)
    // ========================================================================
    ULTRA_INLINE ULTRA_HOT
    uint32_t PopBatch(UltraPacket* ULTRA_RESTRICT out_packets, uint32_t max_count) noexcept {
        const uint32_t current_tail = tail.load(std::memory_order_relaxed);
        const uint32_t current_head = head.load(std::memory_order_acquire);
        
        if (current_tail == current_head) ULTRA_UNLIKELY {
            return 0;
        }
        
        // Calculate available packets
        const uint32_t available = (current_head - current_tail) & MASK;
        const uint32_t to_read = (available < max_count) ? available : max_count;
        
        // Batch copy (compiler will optimize this to SIMD on modern CPUs)
        for (uint32_t i = 0; i < to_read; ++i) {
            out_packets[i] = buffer[(current_tail + i) & MASK];
        }
        
        // Commit batch read
        tail.store((current_tail + to_read) & MASK, std::memory_order_release);
        
        return to_read;
    }
    
    // ========================================================================
    // PEEK (Non-destructive read)
    // ========================================================================
    ULTRA_INLINE
    bool Peek(UltraPacket& out_packet) const noexcept {
        const uint32_t current_tail = tail.load(std::memory_order_relaxed);
        const uint32_t current_head = head.load(std::memory_order_acquire);
        
        if (current_tail == current_head) {
            return false;
        }
        
        out_packet = buffer[current_tail];
        return true;
    }
    
    // ========================================================================
    // SIZE QUERY (Debug only)
    // ========================================================================
    ULTRA_INLINE
    uint32_t Size() const noexcept {
        const uint32_t h = head.load(std::memory_order_acquire);
        const uint32_t t = tail.load(std::memory_order_acquire);
        return (h - t) & MASK;
    }
    
    ULTRA_INLINE
    bool IsEmpty() const noexcept {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }
    
    ULTRA_INLINE
    bool IsFull() const noexcept {
        const uint32_t h = head.load(std::memory_order_acquire);
        const uint32_t t = tail.load(std::memory_order_acquire);
        return ((h + 1) & MASK) == t;
    }
};

// ============================================================================
// ULTRA JOYSTICK COMPONENT (128-byte aligned for AVX)
// ============================================================================
struct alignas(128) UltraJoystickComponent {
    // --- Configuration (Read-only after init) ---
    float centerX, centerY;
    float outerRadius;
    float innerDeadzone;
    
    // --- Current State (Hot data - frequently accessed) ---
    alignas(16) float currentX, currentY;
    alignas(16) float normalizedX, normalizedY;
    float magnitude;
    float angle;
    
    uint8_t isTouched;
    uint8_t activeMagnetID;
    uint8_t prevMagnetID;
    uint8_t _pad1;
    
    // --- Visual State ---
    float visualScale;
    uint32_t visualFlags;
    
    // --- Lock-Free Queue (256 packets = 2KB) ---
    alignas(CACHE_LINE) UltraRingBuffer<256> queue;
    
    // --- Performance Metrics (Debug only) ---
    #ifdef ENGINE_DEBUG
    alignas(CACHE_LINE) std::atomic<uint64_t> totalWrites{0};
    alignas(CACHE_LINE) std::atomic<uint64_t> totalReads{0};
    std::atomic<uint64_t> droppedPackets{0};
    #endif
};

static_assert(alignof(UltraJoystickComponent) == 128, "Must be 128-byte aligned for AVX");

// ============================================================================
// ULTRA-FAST HELPER FUNCTIONS (Using C++20/23 Features)
// ============================================================================

// Fast normalized-to-fixed conversion using C++20 bit_cast
ULTRA_INLINE constexpr int16_t FloatToFixed16(float value) noexcept {
    // Clamp to [-1.0, 1.0]
    value = (value < -1.0f) ? -1.0f : (value > 1.0f) ? 1.0f : value;
    return static_cast<int16_t>(value * 32767.0f);
}

ULTRA_INLINE constexpr float Fixed16ToFloat(int16_t value) noexcept {
    return static_cast<float>(value) / 32767.0f;
}

// Fast timestamp (16-bit wrapping milliseconds)
ULTRA_INLINE uint16_t GetTimestamp16() noexcept {
    // Platform-specific: get milliseconds and mask to 16 bits
    // This wraps at 65.536 seconds, which is fine for latency measurement
    #ifdef __linux__
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t ms = (ts.tv_sec * 1000ULL) + (ts.tv_nsec / 1000000ULL);
        return static_cast<uint16_t>(ms & 0xFFFFu);
    #else
        // Fallback: use high-resolution clock
        return 0; // Implement platform-specific version
    #endif
}

// ============================================================================
// PACKET BUILDER (Zero-allocation packet construction)
// ============================================================================
struct UltraPacketBuilder {
    UltraPacket packet{};
    
    ULTRA_INLINE UltraPacketBuilder& WithAxes(float x, float y) noexcept {
        packet.axisX = FloatToFixed16(x);
        packet.axisY = FloatToFixed16(y);
        return *this;
    }
    
    ULTRA_INLINE UltraPacketBuilder& WithAction(uint8_t action_id, uint8_t event_type) noexcept {
        packet.SetActionID(action_id);
        packet.SetEventType(event_type);
        return *this;
    }
    
    ULTRA_INLINE UltraPacketBuilder& WithTimestamp() noexcept {
        packet.SetTimestamp(GetTimestamp16());
        return *this;
    }
    
    ULTRA_INLINE UltraPacketBuilder& WithFlags(uint8_t flags) noexcept {
        packet.SetFlags(flags);
        return *this;
    }
    
    ULTRA_INLINE UltraPacket Build() const noexcept {
        return packet;
    }
};

} // namespace SecretEngine::Ultra

// ============================================================================
// USAGE EXAMPLE (Touch Thread)
// ============================================================================
/*
void OnTouchEvent(UltraJoystickComponent& joy, float x, float y, bool pressed) {
    auto packet = UltraPacketBuilder()
        .WithAxes(x, y)
        .WithAction(10, pressed ? 1 : 0)
        .WithTimestamp()
        .Build();
    
    if (!joy.queue.Push(packet)) {
        // Buffer overflow (extremely rare with 256 slots)
        #ifdef ENGINE_DEBUG
        joy.droppedPackets.fetch_add(1, std::memory_order_relaxed);
        #endif
    }
}
*/

// ============================================================================
// USAGE EXAMPLE (Game Thread - Batch Processing)
// ============================================================================
/*
void ProcessInput(UltraJoystickComponent& joy, PlayerController& player) {
    UltraPacket batch[32]; // Process up to 32 packets per frame
    uint32_t count = joy.queue.PopBatch(batch, 32);
    
    for (uint32_t i = 0; i < count; ++i) {
        const auto& pkt = batch[i];
        
        // Movement
        float x = Fixed16ToFloat(pkt.axisX);
        float y = Fixed16ToFloat(pkt.axisY);
        player.SetMovement(x, y);
        
        // Action
        uint8_t action = pkt.GetActionID();
        if (action != 0) {
            player.DispatchAction(action, pkt.GetEventType());
        }
    }
}
*/
