#include "Profiler.h"
#include <SecretEngine/ILogger.h>
#include <cstdio>
#include <cmath>
#include <cstring>

#ifdef SE_PLATFORM_ANDROID
#include <sys/sysinfo.h>
#include <fstream>
#endif

namespace SecretEngine {

// ============================================================================
// FRAME LIFECYCLE
// ============================================================================

void Profiler::BeginFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();
    ++m_totalFrames;
    
    // Calculate delta time from last frame
    if (m_lastFrameEnd.time_since_epoch().count() > 0) {
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(
            m_frameStart - m_lastFrameEnd).count();
        float dt = delta / 1000.0f; // Convert to milliseconds
        m_stats.delta_time_ms.store(dt, std::memory_order_relaxed);
        
        // Instant FPS (with safety check for division by zero)
        if (dt > 0.001f) { // Minimum 0.001ms to avoid division issues
            float fps = 1000.0f / dt;
            // Clamp FPS to reasonable range (0-1000)
            if (fps > 1000.0f) fps = 1000.0f;
            if (fps < 0.0f) fps = 0.0f;
            m_stats.fps_instant.store(fps, std::memory_order_relaxed);
            UpdateFPSAverage(fps);
        }
    }
    
    // Reset per-frame counters
    m_stats.ResetFrameCounters();
}

void Profiler::EndFrame() {
    m_lastFrameEnd = std::chrono::high_resolution_clock::now();
    
    // Calculate total frame time
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        m_lastFrameEnd - m_frameStart).count();
    float frameTime = frameDuration / 1000.0f;
    
    // CPU frame time is total minus GPU (if GPU time is available)
    float gpuTime = m_stats.gpu_frame_time.load(std::memory_order_relaxed);
    float cpuTime = frameTime - gpuTime;
    if (cpuTime < 0.0f) cpuTime = frameTime; // Fallback if GPU time unavailable
    m_stats.cpu_frame_time.store(cpuTime, std::memory_order_relaxed);
    
    // Update hardware stats periodically (not every frame - expensive)
    static int frameCounter = 0;
    if (++frameCounter >= 60) { // Every 60 frames (~1 second at 60fps)
        UpdateHardwareStats();
        frameCounter = 0;
    }
}

// ============================================================================
// FPS AVERAGING
// ============================================================================

void Profiler::UpdateFPSAverage(float instantFPS) {
    // Rolling average over last N samples
    m_fpsSamples[m_fpsSampleIndex] = instantFPS;
    m_fpsSampleIndex = (m_fpsSampleIndex + 1) % FPS_SAMPLE_COUNT;
    
    // Calculate average
    float sum = 0.0f;
    for (int i = 0; i < FPS_SAMPLE_COUNT; ++i) {
        sum += m_fpsSamples[i];
    }
    float avg = sum / FPS_SAMPLE_COUNT;
    m_stats.fps_avg_1s.store(avg, std::memory_order_relaxed);
}

// ============================================================================
// HARDWARE MONITORING
// ============================================================================

void Profiler::UpdateHardwareStats() {
#ifdef SE_PLATFORM_ANDROID
    // === NATIVE MEMORY TRACKING (Android) ===
    // Use mallinfo to get actual heap usage
    #include <malloc.h>
    struct mallinfo mi = mallinfo();
    uint64_t heapBytes = mi.uordblks; // Bytes in use
    m_stats.mem_system_allocated.store(heapBytes, std::memory_order_relaxed);
    
    // === BATTERY LEVEL ===
    /* Disabling due to SELinux AVC denials on some devices
    std::ifstream batteryFile("/sys/class/power_supply/battery/capacity");
    if (batteryFile.is_open()) {
        int batteryPct = 0;
        batteryFile >> batteryPct;
        m_stats.hw_battery_pct.store(batteryPct / 100.0f, std::memory_order_relaxed);
        batteryFile.close();
    }
    */
    m_stats.hw_battery_pct.store(1.0f, std::memory_order_relaxed);
    
    // === THERMAL STATUS ===
    /* Disabling due to performance and access restrictions
    std::ifstream thermalFile("/sys/class/thermal/thermal_zone0/temp");
    if (thermalFile.is_open()) {
        int tempMilliC = 0;
        thermalFile >> tempMilliC;
        float tempC = tempMilliC / 1000.0f;
        
        uint32_t status = 0; // NONE
        if (tempC > 70.0f) status = 1; // LIGHT
        if (tempC > 80.0f) status = 2; // SEVERE
        if (tempC > 90.0f) status = 3; // CRITICAL
        
        m_stats.hw_thermal_status.store(status, std::memory_order_relaxed);
        thermalFile.close();
    }
    */
    m_stats.hw_thermal_status.store(0, std::memory_order_relaxed);
    
    // === AVAILABLE MEMORY ===
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        uint64_t availableMB = (info.freeram * info.mem_unit) / (1024 * 1024);
        m_stats.hw_mem_available_mb.store(static_cast<uint32_t>(availableMB), std::memory_order_relaxed);
    }
#else
    // Desktop fallback - set defaults
    m_stats.hw_battery_pct.store(1.0f, std::memory_order_relaxed);
    m_stats.hw_thermal_status.store(0, std::memory_order_relaxed);
    m_stats.hw_mem_available_mb.store(8192, std::memory_order_relaxed); // Assume 8GB
#endif
}

// ============================================================================
// LOGGING & REPORTING
// ============================================================================

void Profiler::LogReport(ILogger* logger) {
    if (!logger) return;
    
    // Check if we should log based on interval
    float dt = m_stats.delta_time_ms.load(std::memory_order_relaxed);
    m_timeSinceLastReport += dt / 1000.0f; // Convert to seconds
    
    if (m_timeSinceLastReport < m_reportInterval) {
        return; // Not time yet
    }
    m_timeSinceLastReport = 0.0f;
    
    // === FORMAT COMPREHENSIVE REPORT (Zero Allocation) ===
    
    // Load all stats (relaxed ordering for reporting)
    float dt_ms = m_stats.delta_time_ms.load(std::memory_order_relaxed);
    float fps_inst = m_stats.fps_instant.load(std::memory_order_relaxed);
    float fps_avg = m_stats.fps_avg_1s.load(std::memory_order_relaxed);
    float cpu_time = m_stats.cpu_frame_time.load(std::memory_order_relaxed);
    float gpu_time = m_stats.gpu_frame_time.load(std::memory_order_relaxed);
    
    uint32_t draws = m_stats.r_draw_calls.load(std::memory_order_relaxed);
    uint32_t tris = m_stats.r_triangles.load(std::memory_order_relaxed);
    uint32_t insts = m_stats.r_instances.load(std::memory_order_relaxed);
    uint32_t vram = m_stats.r_vram_usage_mb.load(std::memory_order_relaxed);
    uint32_t pipeline_binds = m_stats.r_pipeline_binds.load(std::memory_order_relaxed);
    uint32_t descriptor_binds = m_stats.r_descriptor_binds.load(std::memory_order_relaxed);
    
    uint32_t entities = m_stats.l_entities_active.load(std::memory_order_relaxed);
    uint32_t physics = m_stats.l_physics_checks.load(std::memory_order_relaxed);
    uint32_t packets_in = m_stats.l_fda_packets_in.load(std::memory_order_relaxed);
    uint32_t packets_out = m_stats.l_fda_packets_out.load(std::memory_order_relaxed);
    
    uint64_t mem_sys = m_stats.mem_system_allocated.load(std::memory_order_relaxed);
    uint64_t mem_arena = m_stats.mem_arena_peak.load(std::memory_order_relaxed);
    float mem_pool = m_stats.mem_pool_occupancy.load(std::memory_order_relaxed);
    
    float battery = m_stats.hw_battery_pct.load(std::memory_order_relaxed);
    uint32_t thermal = m_stats.hw_thermal_status.load(std::memory_order_relaxed);
    uint32_t mem_avail = m_stats.hw_mem_available_mb.load(std::memory_order_relaxed);
    
    // === PRIMARY PERFORMANCE LOG ===
    int written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
        "[PERF] Frame:%llu | DT:%.1fms | FPS:%.0f(avg:%.0f) | CPU:%.1fms GPU:%.1fms",
        static_cast<unsigned long long>(m_totalFrames),
        dt_ms, fps_inst, fps_avg, cpu_time, gpu_time
    );
    
    if (written > 0 && written < LOG_BUFFER_SIZE) {
        logger->LogInfo("Profiler", m_logBuffer);
    }
    
    // === RENDERER STATS LOG ===
    written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
        "[RENDER] TRI:%uk INST:%u DRAW:%u | PIPE:%u DESC:%u | VRAM:%uMB",
        tris / 1000, insts, draws, pipeline_binds, descriptor_binds, vram
    );
    
    if (written > 0 && written < LOG_BUFFER_SIZE) {
        logger->LogInfo("Profiler", m_logBuffer);
    }
    
    // === LOGIC & PROCESSING LOG ===
    if (entities > 0 || physics > 0 || packets_in > 0 || packets_out > 0) {
        written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
            "[LOGIC] ENT:%u | PHYS:%u | NET_IN:%u NET_OUT:%u",
            entities, physics, packets_in, packets_out
        );
        
        if (written > 0 && written < LOG_BUFFER_SIZE) {
            logger->LogInfo("Profiler", m_logBuffer);
        }
    }
    
    // === MEMORY STATS LOG ===
    written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
        "[MEMORY] SYS:%luMB | ARENA:%luMB | POOL:%.1f%% | RAM_FREE:%uMB",
        static_cast<unsigned long>(mem_sys / (1024 * 1024)),
        static_cast<unsigned long>(mem_arena / (1024 * 1024)),
        mem_pool * 100.0f,
        mem_avail
    );
    
    if (written > 0 && written < LOG_BUFFER_SIZE) {
        logger->LogInfo("Profiler", m_logBuffer);
    }
    
    // === HARDWARE HEALTH LOG ===
    const char* thermalStatus[] = {"NONE", "LIGHT", "SEVERE", "CRITICAL"};
    const char* thermalStr = (thermal < 4) ? thermalStatus[thermal] : "UNKNOWN";
    
    written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
        "[HARDWARE] BATT:%.0f%% | THERMAL:%s",
        battery * 100.0f, thermalStr
    );
    
    if (written > 0 && written < LOG_BUFFER_SIZE) {
        logger->LogInfo("Profiler", m_logBuffer);
    }
    
    // === PERFORMANCE WARNINGS ===
    if (fps_avg < 30.0f) {
        logger->LogWarning("Profiler", "LOW FPS: Average FPS below 30!");
    }
    
    if (cpu_time > 16.6f) {
        written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
            "HIGH CPU TIME: %.1fms (target: 16.6ms for 60fps)", cpu_time);
        if (written > 0 && written < LOG_BUFFER_SIZE) {
            logger->LogWarning("Profiler", m_logBuffer);
        }
    }
    
    if (thermal >= 2) {
        logger->LogWarning("Profiler", "THERMAL WARNING: Device temperature elevated!");
    }
    
    if (mem_avail < 512) {
        written = snprintf(m_logBuffer, LOG_BUFFER_SIZE,
            "LOW MEMORY: Only %uMB available", mem_avail);
        if (written > 0 && written < LOG_BUFFER_SIZE) {
            logger->LogWarning("Profiler", m_logBuffer);
        }
    }
    
    // === SEPARATOR FOR READABILITY ===
    logger->LogInfo("Profiler", "----------------------------------------");
}

void Profiler::LogDetailedReport(ILogger* logger) {
    if (!logger) return;
    
    // This can be called manually for extra detailed debugging
    logger->LogInfo("Profiler", "========================================");
    logger->LogInfo("Profiler", "=== DETAILED PERFORMANCE REPORT ===");
    logger->LogInfo("Profiler", "========================================");
    
    LogReport(logger); // Call standard report
    
    // Add extra details here if needed
    int written = snprintf(m_detailedBuffer, DETAILED_BUFFER_SIZE,
        "Total frames processed: %llu",
        static_cast<unsigned long long>(m_totalFrames)
    );
    
    if (written > 0 && written < DETAILED_BUFFER_SIZE) {
        logger->LogInfo("Profiler", m_detailedBuffer);
    }
}

} // namespace SecretEngine
