# SecretEngine - High-Performance Development Guide

This document defines the technical standards for maintaining maximum performance across the SecretEngine core and its plugins.

## 1. Fast Data Architecture (FDA)
The primary mechanism for high-frequency communication (Input, Logic, Rendering) is the FDA.

### 1.1 The 8-Byte Rule
If a data update happens per frame or per sub-frame (e.g., joystick movement, physics ticks), it **must** be packed into an `UltraPacket`.
- **Constraint**: Never pass large structs or use `std::string` in hot paths.
- **Implementation**: Bit-pack your state into the 24-bit metadata or the two 16-bit data fields.

### 1.2 Lock-Free Pipelines
Always use `UltraRingBuffer` for cross-plugin communication. 
- **SPSC Only**: These buffers are Single-Producer Single-Consumer. If you have multiple producers, use separate buffers or a multiplexer plugin.
- **Cache Alignment**: `alignas(64)` is mandatory for atomic heads/tails to prevent false sharing.

## 2. Memory Management (Zero-Allocation Runtime)
Once the engine enters the `Main Loop`, **zero heap allocations** are allowed in the hot path.

### 2.1 Persistent Mapping
- **Vulkan**: Always use persistently mapped SSBOs/UBOs for data that changes every frame. 
- **Mega Geometry**: The `MegaGeometryRenderer` uses a single `m_instanceDataMapped` pointer. Updating an instance is a simple memory write, not a `vkMapMemory` call.

### 2.2 Arena & Stack
- For temporary per-frame calculations, use the stack or a pre-allocated Arena.
- Avoid `std::vector::push_back` if it triggers a resize. `reserve()` during initialization.

## 3. Data-Oriented Design (DOD)
### 3.1 Cache Locality
- Keep related data together in memory. 
- **Wrong**: `std::vector<EntityPointer>` where each entity is a separate allocation.
- **Right**: `std::vector<TransformComponent>` where all transforms are in a contiguous array.

### 3.2 Branch Optimization
- Use `[[unlikely]]` and `[[likely]]` for error checks in hot loops.
- Use `[[gnu::always_inline]]` for small, high-frequency utility functions.

## 4. Plugin Coding Standards for Performance
When creating a new plugin:
1. **No Virtuals in Hot Loops**: Use interfaces for high-level lifecycle events (`OnLoad`), but avoid virtual calls inside `OnUpdate` loops where possible.
2. **Batching**: If you must process data from an FDA stream, use `PopBatch` to drain the buffer in one go rather than popping individual packets in a slow loop.
3. **SIMD Friendly**: Structure your data so the compiler can autovectorize (e.g., arrays of floats for physics/math).

## 5. Adding New Global Variables / State
If you need to track new engine-wide state (e.g., "Wind Speed", "Global Time"):
- **Variable Type**: Use POD (Plain Old Data) types.
- **Access**: Add it to a `GlobalState` struct in the renderer or logic plugin, and update it via an FDA packet from the source system.
- **Avoid**: Static globals that require mutexes for access.

## 6. Performance Benchmarks (Reference)
On mid-range mobile hardware (Adreno 619):
- **Budget**: 16.6ms (60 FPS) or 8.3ms (120 FPS).
- **Triangle Limit**: Aim to keep visible density high but managed via `MegaGeometry` indirect draws to keep Draw Calls < 10.

---
**Status**: APPROVED STANDARD
**Last Updated**: 2026-02-04
