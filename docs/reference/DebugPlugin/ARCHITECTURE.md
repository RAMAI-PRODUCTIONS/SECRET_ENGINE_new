# DebugPlugin Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      SecretEngine Core                       │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐            │
│  │   ICore    │  │  ILogger   │  │ IRenderer  │            │
│  └─────┬──────┘  └──────┬─────┘  └──────┬─────┘            │
└────────┼─────────────────┼────────────────┼─────────────────┘
         │                 │                │
         │                 │                │
┌────────▼─────────────────▼────────────────▼─────────────────┐
│                      DebugPlugin                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              OnUpdate() - Every Frame                 │   │
│  │  1. Profiler::BeginFrame()                           │   │
│  │  2. Gather renderer stats                            │   │
│  │  3. Update on-screen FPS display                     │   │
│  │  4. Profiler::EndFrame()                             │   │
│  │  5. Profiler::LogReport() - Every 1 second           │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │                   Profiler (Singleton)                │   │
│  │  ┌────────────────────────────────────────────────┐  │   │
│  │  │           EngineStats (Atomics)                │  │   │
│  │  │  • fps_instant, fps_avg_1s                     │  │   │
│  │  │  • cpu_frame_time, gpu_frame_time              │  │   │
│  │  │  • r_draw_calls, r_triangles, r_instances      │  │   │
│  │  │  • mem_system_allocated, mem_arena_peak        │  │   │
│  │  │  • hw_battery_pct, hw_thermal_status           │  │   │
│  │  └────────────────────────────────────────────────┘  │   │
│  │                                                        │   │
│  │  Pre-allocated Buffers:                               │   │
│  │  • m_logBuffer[512]                                   │   │
│  │  • m_detailedBuffer[1024]                             │   │
│  │  • m_fpsSamples[60]                                   │   │
│  └──────────────────────────────────────────────────────┘   │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │  Android Logcat │
                  │  (via ILogger)  │
                  └─────────────────┘
```

## Data Flow

### Frame Processing
```
Frame Start
    │
    ├─► Profiler::BeginFrame()
    │   ├─► Calculate delta time
    │   ├─► Calculate instant FPS
    │   ├─► Update FPS average
    │   └─► Reset frame counters
    │
    ├─► Gather Renderer Stats
    │   ├─► renderer->GetStats(inst, tris, draws)
    │   └─► Update atomic stats
    │
    ├─► Update On-Screen Display
    │   └─► renderer->SetDebugInfo(0, "FPS: 60")
    │
    ├─► Profiler::EndFrame()
    │   ├─► Calculate frame time
    │   ├─► Calculate CPU/GPU split
    │   └─► Update hardware stats (every 60 frames)
    │
    └─► Profiler::LogReport()
        ├─► Check if interval elapsed
        ├─► Format multi-line report
        ├─► Log to logcat via ILogger
        └─► Check warning thresholds
```

## Memory Layout

### DebugPlugin Instance
```
DebugPlugin (Stack/Static)
├─► m_core: ICore*                    [8 bytes]
├─► m_isActive: bool                  [1 byte]
├─► m_frameCount: uint64_t            [8 bytes]
├─► m_lastTime: time_point            [16 bytes]
└─► m_fpsBuffer: char[32]             [32 bytes]
                                Total: ~65 bytes
```

### Profiler Singleton
```
Profiler (Static Singleton)
├─► m_stats: EngineStats              [~200 bytes]
│   └─► All std::atomic members
├─► m_frameStart: time_point          [16 bytes]
├─► m_lastFrameEnd: time_point        [16 bytes]
├─► m_totalFrames: uint64_t           [8 bytes]
├─► m_fpsSamples: float[60]           [240 bytes]
├─► m_fpsSampleIndex: int             [4 bytes]
├─► m_reportInterval: float           [4 bytes]
├─► m_timeSinceLastReport: float      [4 bytes]
├─► m_logBuffer: char[512]            [512 bytes]
└─► m_detailedBuffer: char[1024]      [1024 bytes]
                                Total: ~2KB
```

### Total Memory Footprint
- **Static**: ~2KB (Profiler singleton)
- **Per-Frame Allocation**: 0 bytes (zero-allocation design)
- **Stack Usage**: ~65 bytes (DebugPlugin instance)

## Thread Safety Model

### Atomic Operations
```cpp
// Writer (main thread)
stats.fps_instant.store(60.0f, std::memory_order_relaxed);

// Reader (any thread)
float fps = stats.fps_instant.load(std::memory_order_relaxed);
```

### Why Relaxed Ordering?
- Statistics are for monitoring, not synchronization
- No dependencies between different stats
- Performance critical (every frame)
- Eventual consistency is acceptable

### Thread-Safe Guarantee
```
✅ Multiple readers: Safe (atomic loads)
✅ Single writer: Safe (main thread only)
✅ No locks: Lock-free design
✅ No race conditions: Atomic operations
```

## Logging Pipeline

### Report Generation (Every 1 Second)
```
LogReport()
    │
    ├─► Check interval elapsed
    │   └─► m_timeSinceLastReport >= m_reportInterval
    │
    ├─► Load all stats (atomic loads)
    │   ├─► fps_instant, fps_avg, cpu_time, gpu_time
    │   ├─► draws, tris, insts, vram
    │   ├─► mem_sys, mem_arena, mem_pool
    │   └─► battery, thermal, mem_avail
    │
    ├─► Format [PERF] line
    │   └─► snprintf(m_logBuffer, ...)
    │
    ├─► Log via ILogger
    │   └─► logger->LogInfo("Profiler", m_logBuffer)
    │
    ├─► Format [RENDER] line
    │   └─► snprintf + LogInfo
    │
    ├─► Format [MEMORY] line
    │   └─► snprintf + LogInfo
    │
    ├─► Format [HARDWARE] line
    │   └─► snprintf + LogInfo
    │
    └─► Check warning thresholds
        ├─► if (fps_avg < 30) → LogWarning
        ├─► if (cpu_time > 16.6) → LogWarning
        ├─► if (thermal >= 2) → LogWarning
        └─► if (mem_avail < 512) → LogWarning
```

### ILogger → Android Logcat
```
ILogger::LogInfo("Profiler", message)
    │
    ├─► Acquire mutex lock
    │
    ├─► Determine priority
    │   ├─► INFO → ANDROID_LOG_INFO
    │   ├─► WARN → ANDROID_LOG_WARN
    │   └─► ERROR → ANDROID_LOG_ERROR
    │
    ├─► __android_log_print(priority, "Profiler", message)
    │
    └─► Release mutex lock
```

## Lifecycle

### Plugin Lifecycle
```
Engine Start
    │
    ├─► DebugPlugin::OnLoad(core)
    │   ├─► Store core pointer
    │   ├─► Register capability "debug"
    │   ├─► Initialize profiler
    │   └─► Log startup info
    │
    ├─► DebugPlugin::OnActivate()
    │   ├─► Set m_isActive = true
    │   ├─► Reset frame counter
    │   └─► Log activation
    │
    ├─► DebugPlugin::OnUpdate(dt) [Every Frame]
    │   └─► [See Frame Processing above]
    │
    ├─► DebugPlugin::OnDeactivate()
    │   ├─► Set m_isActive = false
    │   ├─► Log final statistics
    │   └─► Log deactivation
    │
    ├─► DebugPlugin::OnUnload()
    │   ├─► Log unload message
    │   └─► Clear core pointer
    │
    └─► DebugPlugin::~DebugPlugin()
        ├─► if (m_isActive) OnDeactivate()
        └─► if (m_core) OnUnload()
```

### Profiler Lifecycle
```
First Access
    │
    └─► Profiler::Instance()
        └─► Construct static instance
            ├─► Initialize all members
            ├─► Zero all buffers
            └─► Set default interval (1.0s)

Program Exit
    │
    └─► Static destruction
        └─► Profiler::~Profiler()
            └─► Automatic cleanup (no manual work needed)
```

## Warning System

### Threshold Monitoring
```
LogReport() [Every 1 Second]
    │
    ├─► Check FPS
    │   └─► if (fps_avg < 30.0f)
    │       └─► LogWarning("LOW FPS: Average FPS below 30!")
    │
    ├─► Check CPU Time
    │   └─► if (cpu_time > 16.6f)
    │       └─► LogWarning("HIGH CPU TIME: %.1fms", cpu_time)
    │
    ├─► Check Thermal
    │   └─► if (thermal >= 2)  // SEVERE or CRITICAL
    │       └─► LogWarning("THERMAL WARNING: Device temperature elevated!")
    │
    └─► Check Memory
        └─► if (mem_avail < 512)
            └─► LogWarning("LOW MEMORY: Only %uMB available", mem_avail)
```

### Warning Thresholds
| Metric | Warning Level | Critical Level |
|--------|---------------|----------------|
| FPS | < 30 | < 15 |
| CPU Time | > 16.6ms | > 33.3ms |
| Thermal | SEVERE (80°C) | CRITICAL (90°C) |
| Memory | < 512MB | < 256MB |

## Performance Characteristics

### Time Complexity
- `BeginFrame()`: O(1)
- `EndFrame()`: O(1) amortized (hardware stats every 60 frames)
- `LogReport()`: O(1) (fixed number of stats)
- `UpdateFPSAverage()`: O(1) (fixed window size)

### Space Complexity
- Static memory: O(1) - ~2KB fixed
- Per-frame allocation: O(1) - 0 bytes
- Stack usage: O(1) - ~65 bytes

### CPU Impact
- Per-frame overhead: < 0.1ms
- Logging overhead: < 0.5ms (only every 1 second)
- Hardware stats: < 1ms (only every 60 frames)

## Integration Points

### Required Interfaces
```cpp
ICore
├─► GetLogger() → ILogger*
├─► GetCapability("rendering") → IPlugin*
├─► GetWorld() → IWorld*
├─► GetInput() → IInputSystem*
└─► ShouldClose() → bool

ILogger
├─► LogInfo(category, message)
├─► LogWarning(category, message)
└─► LogError(category, message)

IRenderer (via IPlugin)
├─► GetInterface(1) → IRenderer*
├─► GetStats(inst, tris, draws)
└─► SetDebugInfo(slot, text)
```

### Optional Extensions
```cpp
IWorld (future)
└─► GetEntityCount() → uint32_t

IInputSystem (future)
└─► GetEventCount() → uint32_t
```

## Error Handling

### Defensive Checks
```cpp
// Null pointer checks
if (!core) return;
if (!m_core || !m_isActive) return;
if (!logger) return;

// Division by zero
if (dt > 0.001f) {
    float fps = 1000.0f / dt;
}

// Buffer overflow
int written = snprintf(buffer, size, ...);
if (written > 0 && written < size) {
    // Safe to use
}

// Value clamping
if (fps > 1000.0f) fps = 1000.0f;
if (fps < 0.0f) fps = 0.0f;
```

### Graceful Degradation
```
Renderer unavailable → Skip render stats, continue
Logger unavailable → Skip logging, continue
World unavailable → Skip entity stats, continue
```

## Best Practices

### Do's ✅
- Use atomic loads with relaxed ordering
- Pre-allocate all buffers
- Check pointers before dereferencing
- Use snprintf for safe formatting
- Clamp values to reasonable ranges
- Log structured, parseable output

### Don'ts ❌
- Don't allocate memory in hot paths
- Don't use std::string in OnUpdate
- Don't log every frame (use intervals)
- Don't block on I/O operations
- Don't use locks (use atomics)
- Don't assume pointers are valid

## Future Architecture

### Planned Extensions
```
Profiler
├─► Scope tracking system
│   ├─► BeginScope(name)
│   ├─► EndScope(name)
│   └─► Hierarchical timing
│
├─► Custom metrics
│   ├─► RegisterMetric(name, atomic*)
│   └─► Dynamic stat registration
│
└─► Export system
    ├─► ExportCSV(filename)
    ├─► ExportJSON(filename)
    └─► Real-time streaming
```

---

**Version**: 1.0  
**Last Updated**: 2026-02-08  
**Status**: Production Ready ✅
