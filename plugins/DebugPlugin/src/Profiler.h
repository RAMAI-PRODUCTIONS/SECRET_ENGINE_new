#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>

namespace SecretEngine {

// ============================================================================
// PERFORMANCE MONITORING SYSTEM
// Zero-allocation, thread-safe stats aggregation for mobile-first engine
// ============================================================================

// POD struct containing all engine metrics (thread-safe via atomics)
struct EngineStats {
    // === CORE TIMING (The Heartbeat) ===
    std::atomic<float> delta_time_ms{0.0f};
    std::atomic<float> fps_instant{0.0f};
    std::atomic<float> fps_avg_1s{0.0f};
    std::atomic<float> cpu_frame_time{0.0f};
    std::atomic<float> gpu_frame_time{0.0f};
    
    // === RENDERER STATS (Vulkan Usage) ===
    std::atomic<uint32_t> r_draw_calls{0};
    std::atomic<uint32_t> r_triangles{0};
    std::atomic<uint32_t> r_instances{0};
    std::atomic<uint32_t> r_pipeline_binds{0};
    std::atomic<uint32_t> r_descriptor_binds{0};
    std::atomic<uint32_t> r_vram_usage_mb{0};
    
    // === LOGIC & PROCESSING ===
    std::atomic<uint32_t> l_entities_active{0};
    std::atomic<uint32_t> l_physics_checks{0};
    std::atomic<uint32_t> l_fda_packets_in{0};
    std::atomic<uint32_t> l_fda_packets_out{0};
    
    // === MEMORY HYGIENE ===
    std::atomic<uint64_t> mem_system_allocated{0};
    std::atomic<uint64_t> mem_arena_peak{0};
    std::atomic<float> mem_pool_occupancy{0.0f};
    
    // === HARDWARE HEALTH ===
    std::atomic<float> hw_battery_pct{1.0f};
    std::atomic<uint32_t> hw_thermal_status{0}; // 0=NONE, 1=LIGHT, 2=SEVERE, 3=CRITICAL
    std::atomic<uint32_t> hw_mem_available_mb{0};
    
    // Reset per-frame counters (call at BeginFrame)
    void ResetFrameCounters() {
        r_draw_calls.store(0, std::memory_order_relaxed);
        r_pipeline_binds.store(0, std::memory_order_relaxed);
        r_descriptor_binds.store(0, std::memory_order_relaxed);
        l_physics_checks.store(0, std::memory_order_relaxed);
        l_fda_packets_in.store(0, std::memory_order_relaxed);
        l_fda_packets_out.store(0, std::memory_order_relaxed);
    }
};

// ============================================================================
// PROFILER - Core Performance Monitoring Service
// ============================================================================
class Profiler {
public:
    static Profiler& Instance() {
        static Profiler instance;
        return instance;
    }
    
    // Frame lifecycle
    void BeginFrame();
    void EndFrame();
    
    // Access to stats (thread-safe)
    EngineStats& GetStats() { return m_stats; }
    
    // Logging control
    void SetReportInterval(float seconds) { 
        if (seconds > 0.0f) {
            m_reportInterval = seconds; 
        }
    }
    void LogReport(class ILogger* logger);
    void LogDetailedReport(class ILogger* logger); // More verbose logging
    
    // Memory tracking helpers
    void TrackAllocation(uint64_t size) {
        uint64_t current = m_stats.mem_system_allocated.load(std::memory_order_relaxed);
        m_stats.mem_system_allocated.store(current + size, std::memory_order_relaxed);
    }
    
    void TrackDeallocation(uint64_t size) {
        uint64_t current = m_stats.mem_system_allocated.load(std::memory_order_relaxed);
        if (current >= size) {
            m_stats.mem_system_allocated.store(current - size, std::memory_order_relaxed);
        }
    }
    
    void UpdateArenaMemory(uint64_t peakBytes) {
        m_stats.mem_arena_peak.store(peakBytes, std::memory_order_relaxed);
    }
    
    // Scope-based timing (RAII) - Enhanced with name tracking
    class ScopeTimer {
    public:
        ScopeTimer(const char* name, std::atomic<float>* target)
            : m_name(name)
            , m_target(target)
            , m_start(std::chrono::high_resolution_clock::now())
        {
        }
        
        ~ScopeTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
            if (m_target) {
                m_target->store(duration / 1000.0f, std::memory_order_relaxed);
            }
        }
        
        const char* GetName() const { return m_name; }
        
    private:
        const char* m_name;
        std::atomic<float>* m_target;
        std::chrono::high_resolution_clock::time_point m_start;
    };
    
    // Get frame statistics
    uint64_t GetTotalFrames() const { return m_totalFrames; }
    float GetAverageFPS() const { return m_stats.fps_avg_1s.load(std::memory_order_relaxed); }
    
private:
    Profiler() 
        : m_totalFrames(0)
        , m_reportInterval(1.0f)
        , m_timeSinceLastReport(0.0f)
        , m_fpsSampleIndex(0)
    {
        std::memset(m_fpsSamples, 0, sizeof(m_fpsSamples));
        std::memset(m_logBuffer, 0, sizeof(m_logBuffer));
        std::memset(m_detailedBuffer, 0, sizeof(m_detailedBuffer));
    }
    
    ~Profiler() = default;
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    
    void UpdateFPSAverage(float dt);
    void UpdateHardwareStats();
    void LogMemoryStats(class ILogger* logger);
    void LogRenderStats(class ILogger* logger);
    
    EngineStats m_stats;
    
    // Frame timing
    std::chrono::high_resolution_clock::time_point m_frameStart;
    std::chrono::high_resolution_clock::time_point m_lastFrameEnd;
    uint64_t m_totalFrames;
    
    // FPS averaging (1 second window)
    static constexpr int FPS_SAMPLE_COUNT = 60;
    float m_fpsSamples[FPS_SAMPLE_COUNT];
    int m_fpsSampleIndex;
    
    // Logging control
    float m_reportInterval;
    float m_timeSinceLastReport;
    
    // Pre-allocated buffers for log formatting (zero allocation)
    static constexpr int LOG_BUFFER_SIZE = 512;
    static constexpr int DETAILED_BUFFER_SIZE = 1024;
    char m_logBuffer[LOG_BUFFER_SIZE];
    char m_detailedBuffer[DETAILED_BUFFER_SIZE];
};

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================
#define PROFILE_SCOPE(name, target) \
    SecretEngine::Profiler::ScopeTimer _timer_##name(#name, target)

#define PROFILE_CPU_FRAME() \
    PROFILE_SCOPE(CPUFrame, &SecretEngine::Profiler::Instance().GetStats().cpu_frame_time)

} // namespace SecretEngine
