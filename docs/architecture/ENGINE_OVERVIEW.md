SecretEngine – Engine Overview
Purpose of This Document

This document explains how SecretEngine works at runtime, at a high level, without code.
It defines engine flow, plugin interaction, and data movement.

This document exists to prevent:

incorrect assumptions

invented lifecycles

architectural hallucinations (human or LLM)

1. What SecretEngine Is

SecretEngine is a mobile-first, Vulkan-based, plugin-driven game engine designed to:

ship commercial games

prioritize gameplay feel

minimize engine size and complexity

remain maintainable by a small team (or solo founder)

SecretEngine is not editor-centric and not framework-heavy.

2. High-Level Architecture

SecretEngine is composed of four conceptual layers:

┌──────────────────────────┐
│        Game Data         │  (Scenes, assets, configs)
├──────────────────────────┤
│         Plugins          │  (Renderer, Input, Physics, etc.)
├──────────────────────────┤
│           Core           │  (Interfaces, lifecycle, data flow)
├──────────────────────────┤
│        Platform          │  (Android, Windows entry points)
└──────────────────────────┘


Core defines what can exist

Plugins define how things are done

Platform boots the engine

Game data drives behavior

3. Engine Startup Sequence

Platform entry point starts

Core initializes memory, logging

Job system initializes (auto-detects CPU cores)

Plugin manager scans available plugins

Plugins are loaded (not activated)

Plugin capabilities are registered

Engine validates required capabilities

Plugins are activated according to config

Performance monitoring starts (DebugPlugin)

Initial scene is loaded

Main loop begins

At no point does core assume:

a renderer exists

input exists

networking exists

4. Main Runtime Loop (Conceptual)

Each frame follows this strict order:

Platform events collected

Input sampled (raw)

Input mapped to actions

Simulation step

Visibility & culling update

Fast Data Stream Processing (FDA Drain)
Render submission

UI submission

Platform presentation
Fast Data Stream Flush (FDA Source)

Important:
Rendering never drives gameplay timing.

5. Plugin Interaction Model

Plugins never talk to each other directly

All interaction goes through core-defined interfaces

Core owns lifecycle, not plugins

Plugins may be:

enabled

disabled

replaced

swapped (within safety rules)

6. Data Flow Philosophy

Data flows downward

Control flows upward

Core mediates everything

There are no circular dependencies.

### 6.1 Fast Data Architecture (FDA)
For real-time simulation and rendering, SecretEngine bypasses the standard query/event system in favor of **8-byte packet streams**. This ensures that high-frequency data (Input -> Logic -> GPU) never hits the heap and never stalls a core.

7. Asset Flow

Authoring assets (JSON, GLTF, JPG, etc.)

Asset cooker detects changes

Cooker outputs binary runtime assets

Runtime loads only cooked assets

No runtime parsing of authoring formats

8. Multiplayer Model

Multiplayer is external infrastructure

Engine communicates via messages

Game logic is not network-driven

Offline-first design

9. What This Document Forbids

Hidden global state

Implicit engine flow

Plugin-to-plugin coupling

Renderer-driven gameplay

Editor-required runtime behavior

Status

Frozen
Any change must be reflected in DECISION_LOG.md.


---

## 10. Performance Monitoring (NEW - February 2026)

SecretEngine includes comprehensive, zero-allocation performance monitoring:

### Real-Time Metrics
- Frame timing and FPS (instant + 1-second average)
- Rendering statistics (triangles, instances, draw calls)
- Memory tracking (native heap, VRAM, system RAM)
- Hardware health (battery, thermal status)

### Implementation
- **DebugPlugin**: Aggregates stats from all systems
- **Profiler**: Thread-safe atomic counters, zero allocation
- **Logcat Output**: Structured 5-6 lines per second

### Output Example
```
[PERF] Frame:3046 | DT:20.8ms | FPS:48(avg:49) | CPU:0.1ms GPU:0.0ms
[RENDER] TRI:6195k INST:4001 DRAW:2 | PIPE:0 DESC:0 | VRAM:37MB
[MEMORY] SYS:45MB | ARENA:0MB | POOL:0.0% | RAM_FREE:66MB
[HARDWARE] BATT:100% | THERMAL:NONE
```

**See**: `docs/architecture/PERFORMANCE_MONITORING.md`

---

## 11. Multithreading Architecture (NEW - February 2026)

SecretEngine includes a lock-free job system for multi-core utilization:

### Design
- **Lock-Free Ring Buffer**: 4096 job queue, atomic head/tail
- **Work Stealing**: Efficient load balancing across cores
- **Zero Allocation**: All memory pre-allocated
- **Simple API**: `ParallelFor` for easy data parallelism

### Usage Example
```cpp
// Parallelize instance updates
ParallelFor(4000, [](uint32_t i) {
    UpdateInstance(i);
});
```

### Performance
- **Expected Speedup**: 3-4x on 4-core devices
- **Current Status**: Implemented but not yet integrated
- **Integration**: Add `JobSystem::Instance().Initialize()` to Core::Initialize()

**See**: `docs/architecture/MULTITHREADING_ARCHITECTURE.md`

---

## 12. Current Performance Metrics (February 8, 2026)

### Actual Device Performance (Adreno 619 GPU)
```
Triangles: 6.195M per frame
Instances: 4,001
Draw Calls: 2 (GPU instancing)
FPS: 48-49 average
VRAM: 37MB (well under 19MB danger zone)
```

### Optimization Opportunities
1. **Multithreading** (ready): 3-4x CPU speedup expected
2. **CPU/GPU Timing**: Needs calibration
3. **Native Memory**: Using mallinfo() for tracking

**See**: `docs/CURRENT_ENGINE_STATUS.md`
