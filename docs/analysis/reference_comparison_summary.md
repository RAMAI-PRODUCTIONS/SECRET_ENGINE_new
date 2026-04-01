# Reference System Analysis & Best Practices Summary

## Overview
Analysis of two reference systems against current SECRET_ENGINE implementation:
1. **v7_3_ultra_full** - Level/entity data structure reference
2. **vulkan_rules_improved** - Comprehensive Vulkan 1.4 + C++26 development rules

---

## 1. Level System & JSON Structure Analysis

### Current System (SECRET_ENGINE)
- **Plugin-based**: `LevelSystem` plugin with basic manifest
- **Capabilities**: level_system, level_streaming, level_transitions
- **Structure**: Minimal JSON schema definition

### v7_3_ultra_full Reference System
**Strengths to adopt:**

#### Hierarchical Level Structure
```json
{
  "version": "7.3",
  "name": "ultra_full_scale",
  "core": {
    "transform_settings": {
      "rotation_format": "euler",
      "rotation_unit": "degrees", 
      "rotation_order": "YXZ"
    }
  },
  "references": {
    "players": "entities/players.json",
    "chunks": ["chunks/chunk_0.json", ...]
  }
}
```

**Key improvements:**
- ✅ **Versioning system** - Track level format versions
- ✅ **Core settings** - Centralized transform conventions
- ✅ **Reference-based architecture** - Separate files for entities/chunks
- ✅ **Chunk-based streaming** - Natural fit for level_streaming capability

#### Entity/Instance Data Model
From chunk files, each instance includes:
- `transform`: position, rotation, scale
- `tags`: type classification
- `material_params`: per-instance material overrides
- `culling`: radius for frustum culling
- `lod`: multi-level distance thresholds

**Recommendations:**
1. Add versioning to level JSON schema
2. Implement chunk-based level organization
3. Standardize transform settings globally
4. Add LOD and culling metadata to entities
5. Support material parameter overrides per instance

---

## 2. Vulkan Rules & Architecture Best Practices

### Current System Status
- Android NDK project structure exists
- Vulkan integration present (based on file tree)
- C++ codebase

### vulkan_rules_improved Reference - Critical Adoptions

#### A. Vulkan 1.4 Core Features (No Extension Checks Needed)
**MANDATORY upgrades:**
- `vkCmdPushDescriptorSet` - Eliminate descriptor pool management
- `vkCmdBeginRendering`/`vkCmdEndRendering` - Remove VkRenderPass objects
- `vkCmdPipelineBarrier2` - Modern synchronization
- `VK_INDEX_TYPE_UINT8` - 8-bit indices for small meshes
- `vkCopyMemoryToImageEXT` - Direct CPU→GPU uploads
- `vkMapMemory2` - Replace old mapping API

#### B. C++26 Language Features
**High-value additions:**
```cpp
// Stack-resident dynamic arrays (no heap allocation)
std::inplace_vector<VkWriteDescriptorSet, 16> descriptorWrites;

// Compile-time binary embedding (eliminates AAssetManager for shaders)
#embed "shader.spv"

// Pack indexing (cleaner variadic templates)
Args...[I]

// Self-documenting deleted functions
Foo(const Foo&) = delete("Foo is move-only — use std::move");

// Placeholder in structured bindings
auto [result, _] = twoValueReturn();
```

**MANDATORY replacements:**
- `std::jthread` + `std::stop_token` → Replace all `std::thread`
- `std::expected<T, E>` → All error handling (no exceptions)
- `std::flat_map`/`std::flat_set` → Replace `std::map`/`std::set`
- `std::mdspan` → Multi-dimensional buffer views

#### C. Memory Management Rules
**VMA 3.1+ requirements:**
```cpp
// BANNED: Old usage patterns
VMA_MEMORY_USAGE_GPU_ONLY  // Deprecated

// REQUIRED: New patterns
VMA_MEMORY_USAGE_AUTO + VMA_ALLOCATION_CREATE_*_BIT flags
vmaGetHeapBudgets() // Check before large allocations
```

**Per-frame allocation strategy:**
- `std::pmr::monotonic_buffer_resource` for frame-local allocations
- Pre-allocate all command buffers, staging buffers, query pools
- Zero heap allocation in main loop

#### D. Game Architecture - ECS
**MANDATORY patterns:**
- ECS as primary architecture (entt recommended)
- Separate components: Transform, Render, Physics, Audio
- Fixed-timestep accumulator for deterministic simulation
- Triple-buffered game state ring for render thread
- Job system with `std::jthread` workers

**Threading model:**
```cpp
// Separate threads
- Main/Logic thread
- Render thread  
- Physics thread
- Audio thread (high priority)
- Asset loading thread pool
```

#### E. Android-Specific Rules
**GameActivity (not NativeActivity):**
- Entry point: `void android_main(struct android_app* app)`
- Input: `android_app_swap_input_buffers()`
- Thermal monitoring: `AThermalManager_getThermalHeadroom()`

**Build requirements:**
- NDK r29, Clang 19
- `-std=c++26 -march=armv8.2-a+dotprod+fp16`
- `-fno-exceptions -fno-rtti -flto=thin`
- `targetSdk 35`, `minSdk 24`

#### F. Performance Budgets
**Frame targets:**
- 60 Hz: <16.67ms total (<12ms GPU, <10ms CPU)
- Draw calls: <2000/frame mid-range, <500 minimum spec
- Pipeline changes: <500/frame mid-range

**Profiling tools:**
- `VK_QUERY_TYPE_TIMESTAMP` for GPU timing
- Perfetto for CPU traces
- RenderDoc for GPU analysis
- AGI (Adreno) / Arm Mobile Studio (Mali)

---

## 3. Recommended Implementation Priorities

### Phase 1: Level System Enhancement (High Priority)
1. **Add versioning to level JSON**
   - Schema version field
   - Migration path for format changes

2. **Implement chunk-based organization**
   - Split large levels into streamable chunks
   - Reference-based entity loading
   - Aligns with existing `level_streaming` capability

3. **Standardize transform conventions**
   - Global rotation format/unit/order settings
   - Consistent across all entity types

4. **Add LOD and culling metadata**
   - Per-entity culling radius
   - Distance-based LOD levels
   - Material parameter overrides

### Phase 2: Vulkan Modernization (Critical)
1. **Upgrade to Vulkan 1.4 core features**
   - Remove extension checks for promoted features
   - Adopt dynamic rendering (no VkRenderPass)
   - Use push descriptors for per-object data

2. **VMA 3.1+ migration**
   - Replace deprecated usage patterns
   - Implement budget monitoring
   - Add dedicated allocations for large resources

3. **Synchronization2 adoption**
   - Replace all `vkCmdPipelineBarrier` with `vkCmdPipelineBarrier2`
   - Precise stage/access masks

### Phase 3: C++26 Language Features (Medium Priority)
1. **Error handling modernization**
   - Replace all error codes with `std::expected<T, E>`
   - Document error propagation patterns

2. **Container upgrades**
   - `std::flat_map`/`std::flat_set` for hot paths
   - `std::inplace_vector` for frame-local lists
   - `std::mdspan` for texture/buffer views

3. **Threading improvements**
   - Migrate to `std::jthread` everywhere
   - Implement cooperative cancellation

### Phase 4: Architecture Refinement (Long-term)
1. **ECS implementation**
   - Integrate entt or custom ECS
   - Separate component types
   - Job system with worker pools

2. **GameActivity migration**
   - Replace NativeActivity if currently used
   - Implement proper lifecycle handling
   - Add thermal monitoring

3. **Performance infrastructure**
   - GPU timestamp queries
   - Perfetto integration
   - Frame pacing monitoring

---

## 4. JSON Schema Recommendations

### Proposed Level Schema v1.0
```json
{
  "schema_version": "1.0",
  "level_name": "main_level",
  "metadata": {
    "author": "string",
    "created": "ISO8601",
    "engine_version": "0.1.0"
  },
  "core_settings": {
    "transform": {
      "rotation_format": "euler|quaternion",
      "rotation_unit": "degrees|radians",
      "rotation_order": "YXZ|XYZ|..."
    },
    "rendering": {
      "default_lod_distances": [25, 75, 150],
      "culling_enabled": true
    }
  },
  "references": {
    "entities": ["entities/players.json", "entities/npcs.json"],
    "chunks": ["chunks/chunk_0.json", "chunks/chunk_1.json"],
    "navigation": "nav/navmesh.json"
  },
  "streaming": {
    "chunk_size": 100,
    "load_radius": 2,
    "unload_radius": 3
  }
}
```

### Proposed Entity Schema
```json
{
  "entity_id": "player_0",
  "type": "player|npc|mesh|light|...",
  "transform": {
    "position": [x, y, z],
    "rotation": [x, y, z],
    "scale": [x, y, z]
  },
  "components": {
    "render": {
      "mesh": "path/to/mesh",
      "material": "path/to/material",
      "material_params": {},
      "lod": {
        "enabled": true,
        "levels": [
          {"distance": 25, "mesh": "lod0.mesh"},
          {"distance": 75, "mesh": "lod1.mesh"}
        ]
      }
    },
    "physics": {
      "type": "static|dynamic|kinematic",
      "collision_shape": "box|sphere|mesh"
    },
    "gameplay": {
      "tags": ["enemy", "boss"],
      "stats": {"health": 100}
    }
  },
  "culling": {
    "enabled": true,
    "radius": 1.0,
    "method": "sphere|aabb"
  }
}
```

---

## 5. Compliance Checklist

### Level System
- [ ] Add schema versioning
- [ ] Implement chunk-based streaming
- [ ] Standardize transform settings
- [ ] Add LOD metadata
- [ ] Add culling metadata
- [ ] Support material parameter overrides
- [ ] Document migration path

### Vulkan
- [ ] Set `apiVersion = VK_MAKE_API_VERSION(0,1,4,0)`
- [ ] Remove extension checks for core 1.4 features
- [ ] Adopt `vkCmdBeginRendering` (no VkRenderPass)
- [ ] Use `vkCmdPipelineBarrier2` everywhere
- [ ] Migrate to VMA 3.1+ with `VMA_MEMORY_USAGE_AUTO`
- [ ] Implement `vkCmdPushDescriptorSet` for per-object data
- [ ] Add GPU timestamp queries
- [ ] Enable validation layers in debug only

### C++26
- [ ] `-std=c++26` in CMakeLists.txt
- [ ] Replace `std::thread` with `std::jthread`
- [ ] Adopt `std::expected` for error handling
- [ ] Use `std::flat_map`/`std::flat_set`
- [ ] Add `std::inplace_vector` for frame-local data
- [ ] Use `#embed` for static shaders
- [ ] `-fno-exceptions -fno-rtti` enforced

### Android
- [ ] NDK r29 configured
- [ ] GameActivity (not NativeActivity)
- [ ] `targetSdk 35`, `minSdk 24`
- [ ] `-march=armv8.2-a+dotprod+fp16`
- [ ] Thermal monitoring integrated
- [ ] Perfetto tracing enabled

---

## 6. Key Takeaways

### From v7_3_ultra_full:
1. **Versioned schemas prevent breaking changes**
2. **Chunk-based organization enables streaming**
3. **Reference architecture keeps files manageable**
4. **Per-instance metadata (LOD, culling) improves performance**
5. **Centralized settings ensure consistency**

### From vulkan_rules_improved:
1. **Vulkan 1.4 eliminates extension boilerplate**
2. **C++26 features reduce boilerplate and improve safety**
3. **VMA 3.1+ patterns are mandatory for modern mobile**
4. **ECS architecture scales better than OOP hierarchies**
5. **Profiling infrastructure must be built-in from day one**
6. **Android-specific optimizations are non-negotiable**

### Critical Path:
1. Level schema versioning (prevents future pain)
2. Vulkan 1.4 core adoption (removes technical debt)
3. VMA 3.1+ migration (memory safety)
4. `std::expected` error handling (code clarity)
5. Chunk-based streaming (performance)

---

## 7. Next Steps

1. **Review this document** with the team
2. **Prioritize phases** based on current project state
3. **Create migration tasks** for each checklist item
4. **Update coding standards** document
5. **Set up CI checks** for new rules
6. **Document breaking changes** for existing code

---

*Generated: 2026-03-31*
*References: v7_3_ultra_full, vulkan_rules_improved*
*Target: SECRET_ENGINE modernization*
