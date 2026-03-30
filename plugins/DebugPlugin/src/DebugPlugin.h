#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IRenderer.h>
#include "Profiler.h"
#include "../../core/src/SystemAllocator.h"
#include <memory>
#include <chrono>
#include <cstring>

namespace SecretEngine {
    class DebugPlugin : public IPlugin {
    public:
        DebugPlugin() 
            : m_core(nullptr)
            , m_isActive(false)
            , m_frameCount(0)
            , m_fpsBuffer{0}
        {
            // Pre-allocate FPS string buffer to avoid runtime allocations
            std::memset(m_fpsBuffer, 0, sizeof(m_fpsBuffer));
        }

        ~DebugPlugin() override {
            // Ensure cleanup on destruction
            if (m_isActive) {
                OnDeactivate();
            }
            if (m_core) {
                OnUnload();
            }
        }

        const char* GetName() const override { return "DebugPlugin"; }
        uint32_t GetVersion() const override { return 1; }

        void OnLoad(ICore* core) override {
            if (!core) {
                return; // Defensive: null core check
            }
            
            m_core = core;
            m_core->RegisterCapability("debug", this);
            
            // Initialize profiler with detailed logging
            Profiler::Instance().SetReportInterval(1.0f);
            
            // Log startup information
            ILogger* logger = m_core->GetLogger();
            if (logger) {
                logger->LogInfo("DebugPlugin", "========================================");
                logger->LogInfo("DebugPlugin", "=== DEBUG PLUGIN v1.0 LOADED ===");
                logger->LogInfo("DebugPlugin", "========================================");
                logger->LogInfo("DebugPlugin", "Features: Performance profiling, Memory tracking, Hardware monitoring");
                logger->LogInfo("DebugPlugin", "Logcat interval: 1.0s | Thread-safe: YES | Zero-alloc: YES");
                logger->LogInfo("DebugPlugin", "Metrics: FPS, Frame time, Draw calls, Triangles, Memory, Battery");
            }
        }

        void OnActivate() override {
            m_isActive = true;
            m_frameCount = 0;
            m_lastTime = std::chrono::high_resolution_clock::now();
            
            ILogger* logger = m_core ? m_core->GetLogger() : nullptr;
            if (logger) {
                logger->LogInfo("DebugPlugin", "========================================");
                logger->LogInfo("DebugPlugin", "=== DEBUG PLUGIN ACTIVATED ===");
                logger->LogInfo("DebugPlugin", "========================================");
                logger->LogInfo("DebugPlugin", "Starting performance monitoring...");
            }
        }
        
        void OnDeactivate() override {
            if (!m_isActive) return;
            
            m_isActive = false;
            
            ILogger* logger = m_core ? m_core->GetLogger() : nullptr;
            if (logger) {
                // Log final statistics
                auto& stats = Profiler::Instance().GetStats();
                float avgFps = stats.fps_avg_1s.load(std::memory_order_relaxed);
                
                logger->LogInfo("DebugPlugin", "========================================");
                logger->LogInfo("DebugPlugin", "=== DEBUG PLUGIN DEACTIVATED ===");
                logger->LogInfo("DebugPlugin", "========================================");
                
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Total frames processed: %llu", 
                         static_cast<unsigned long long>(m_frameCount));
                logger->LogInfo("DebugPlugin", buffer);
                
                snprintf(buffer, sizeof(buffer), "Average FPS: %.1f", avgFps);
                logger->LogInfo("DebugPlugin", buffer);
            }
        }
        
        void OnUnload() override {
            ILogger* logger = m_core ? m_core->GetLogger() : nullptr;
            if (logger) {
                logger->LogInfo("DebugPlugin", "========================================");
                logger->LogInfo("DebugPlugin", "=== DEBUG PLUGIN UNLOADED ===");
                logger->LogInfo("DebugPlugin", "========================================");
            }
            
            // Clear core reference
            m_core = nullptr;
        }

        void OnUpdate(float dt) override {
            if (!m_core || !m_isActive) return;
            
            ++m_frameCount;
            
            // === BEGIN FRAME PROFILING ===
            Profiler::Instance().BeginFrame();
            
            auto& stats = Profiler::Instance().GetStats();
            ILogger* logger = m_core->GetLogger();
            
            // === GATHER RENDERER STATS ===
            auto rendering = m_core->GetCapability("rendering");
            if (rendering) {
                auto renderer = static_cast<IRenderer*>(rendering->GetInterface(1));
                if (renderer) {
                    uint32_t inst = 0, tris = 0, draws = 0;
                    renderer->GetStats(inst, tris, draws);
                    
                    // Update profiler stats (for logcat reporting)
                    stats.r_instances.store(inst, std::memory_order_relaxed);
                    stats.r_triangles.store(tris, std::memory_order_relaxed);
                    stats.r_draw_calls.store(draws, std::memory_order_relaxed);
                    
                    // === TRACK NATIVE MEMORY ===
                    uint64_t nativeMemory = SystemAllocator::GetTotalAllocated();
                    stats.mem_system_allocated.store(nativeMemory, std::memory_order_relaxed);
                    
                    // === TRACK VRAM ===
                    uint64_t vramBytes = renderer->GetVRAMUsage();
                    uint32_t vramMB = static_cast<uint32_t>(vramBytes / (1024 * 1024));
                    stats.r_vram_usage_mb.store(vramMB, std::memory_order_relaxed);
                    
                    // === ON-SCREEN UI: FPS ONLY (zero-allocation) ===
                    int fps = static_cast<int>(stats.fps_instant.load(std::memory_order_relaxed));
                    snprintf(m_fpsBuffer, sizeof(m_fpsBuffer), "FPS: %d", fps);
                    
                    renderer->SetDebugInfo(0, m_fpsBuffer);
                    renderer->SetDebugInfo(1, nullptr); // Clear other slots
                    renderer->SetDebugInfo(2, nullptr);
                    renderer->SetDebugInfo(3, nullptr);
                    
                    // Log detailed renderer info every 5 seconds
                    if (m_frameCount % 300 == 0 && logger) {
                        char buffer[256];
                        snprintf(buffer, sizeof(buffer), 
                                "[RENDERER] Instances: %u | Triangles: %u | Draw calls: %u",
                                inst, tris, draws);
                        logger->LogInfo("DebugPlugin", buffer);
                    }
                } else if (logger && m_frameCount == 1) {
                    logger->LogWarning("DebugPlugin", "Renderer interface not available");
                }
            } else if (logger && m_frameCount == 1) {
                logger->LogWarning("DebugPlugin", "Rendering capability not registered");
            }
            
            // === GATHER WORLD STATS ===
            auto world = m_core->GetWorld();
            if (world) {
                // Future: Implement entity counting
                // stats.l_entities_active.store(world->GetEntityCount(), std::memory_order_relaxed);
                
                // Log world info periodically
                if (m_frameCount % 300 == 0 && logger) {
                    logger->LogInfo("DebugPlugin", "[WORLD] Active and processing");
                }
            }
            
            // === GATHER INPUT STATS ===
            auto input = m_core->GetInput();
            if (input && m_frameCount % 300 == 0 && logger) {
                logger->LogInfo("DebugPlugin", "[INPUT] System active");
            }
            
            // === LOG ENGINE LIFECYCLE ===
            if (m_frameCount % 600 == 0 && logger) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), 
                        "[ENGINE] Frame: %llu | Running: %s | Renderer Ready: YES",
                        static_cast<unsigned long long>(m_frameCount),
                        m_core->ShouldClose() ? "NO" : "YES");
                logger->LogInfo("DebugPlugin", buffer);
            }
            
            // === END FRAME PROFILING ===
            Profiler::Instance().EndFrame();
            
            // === LOG DETAILED PERFORMANCE REPORT TO LOGCAT ===
            // This outputs comprehensive stats every 1 second
            Profiler::Instance().LogReport(logger);
        }

    private:
        ICore* m_core;
        bool m_isActive;
        uint64_t m_frameCount;
        std::chrono::high_resolution_clock::time_point m_lastTime;
        
        // Pre-allocated buffer for FPS string (avoid runtime allocation)
        char m_fpsBuffer[32];
    };
}
