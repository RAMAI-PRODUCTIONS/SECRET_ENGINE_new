# Vertex GI System v2.0 - Implementation Summary

**Status:** Ready for Phase 1 implementation  
**Target:** 240 FPS · Zero heap allocation · ≤20 draw calls · Fully dynamic lighting

---

## Key Improvements from v2 Inspiration

### 1. Architecture
- **GI is 100% compute-only** → Contributes ZERO draw calls
- **VMA 3.2.0 pre-allocated pools** → No runtime heap allocation
- **Temporal Accumulation (TAGI)** → Update 1/8 vertices per frame
- **ADPF thermal throttling** → Scales 0-4 tiles/frame based on temperature

### 2. Data Format Upgrades
- **LightVertex: 48 bytes** (cache-line friendly, oct-encoded normals)
- **Vertex colors: R11G11B10F** (HDR range, 4 bytes per vertex)
- **Normal oct-encoding** → Saves 8 bytes per LightVertex (12→4 bytes)

### 3. Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| FPS | 240+ | Was 560, expect 720+ after Phase 1 |
| Draw calls | 0 | GI is compute-only |
| VRAM overhead | ~19.4 MB | Replaces 4-64MB lightmaps |
| Frame time | -0.36ms | Net gain (offloads fragment) |
| GI update (TAGI) | 0.04ms | Per frame (1/8 tile) |

---

## Implementation Phases

### Phase 1: GPU-Driven Vertex Lighting (3-4 days)
**Goal:** Replace fragment lighting with pre-computed vertex colors

**What to implement:**
1. Add `uint vertexLight` field to Vertex struct (R11G11B10F packed)
2. Create `LightUBO` with 16-light capacity
3. Implement `vertex_lighting_p1.comp` compute shader
4. Simplify `mega_geometry.frag` → remove all lighting, just sample textures
5. Normal maps add detail only (micro-occlusion), no re-lighting

**Expected result:** FPS 560 → 720+



### Phase 2: Light Vertex Cache (1 week)
**Goal:** Add one-bounce indirect illumination

**What to implement:**
1. `LightVertex` struct (48 bytes, oct-encoded normals)
2. `LightPathTracer` (CPU, runs on light-dirty flag)
3. `SpatialHash` (zero-allocation, open-addressing)
4. `vertex_merge.comp` (GPU merges camera vertices with light vertices)
5. Dirty-flag system (only update when lights change)

**Expected result:** Indirect lighting visible, GI updates in ≤8 frames

### Phase 3: Temporal Accumulation GI (4-5 days)
**Goal:** Hide per-frame GI cost behind temporal blending

**What to implement:**
1. `TAGIScheduler` (updates 1/8 of vertices per frame)
2. `temporal_blend.comp` (exponential moving average)
3. History buffer (same size as vertex light buffer)
4. `ADPFGIThrottle` (thermal-aware tile count)

**Expected result:** GI cost <0.1ms per frame, invisible at 240fps

### Phase 4: Probe Grid GI (1.5-2 weeks, optional)
**Goal:** AAA-quality indirect lighting for large open scenes

**What to implement:**
1. `IrradianceProbe` struct (SH9 coefficients, 128 bytes)
2. `ProbeGrid` layout (e.g., 16×8×16 = 2048 probes)
3. Probe sampling in vertex shader (trilinear interpolation)
4. Blend probe GI with vertex-merged GI

**Expected result:** Decima-quality GI for open worlds

---

## Core Data Structures

### LightVertex (48 bytes)
```cpp
struct alignas(16) LightVertex {
    glm::vec3  position;        // 12 bytes
    float      mergeRadius;     //  4 bytes
    glm::vec3  throughput;      // 12 bytes (HDR)
    uint32_t   packed_normal;   //  4 bytes (oct-encoded)
    uint16_t   lightIndex;      //  2 bytes
    uint8_t    pathLength;      //  1 byte
    uint8_t    flags;           //  1 byte
    float      connectionPdf;   //  4 bytes (MIS)
    float      mergingPdf;      //  4 bytes (MIS)
};
```

### Vertex Color (R11G11B10F, 4 bytes)
```cpp
// Packed HDR format: R=11 bits, G=11 bits, B=10 bits
// Range: 0–65504 (supports bright lights)
// Vulkan format: VK_FORMAT_B10G11R11_UFLOAT_PACK32
uint32_t vertexLight;  // Replaces RGBA8
```

### Spatial Hash (Zero-Allocation)
```cpp
class SpatialHash {
    // Open-addressing hash, no heap allocation after init
    // Pre-allocated from VMA linear block
    void init(void* preallocatedMem, size_t byteSize, float cellSize);
    void build(const LightVertex* verts, uint32_t count);
    uint32_t query(const glm::vec3& center, float radius,
                   uint32_t* outIndices, uint32_t maxOut) const;
};
```

---

## Key Algorithms

### 1. Hemisphere Ambient (No Lightmaps)
```glsl
// In vertex_lighting_p1.comp
float upBlend = N.y * 0.5 + 0.5;
vec3 ambient = mix(groundColor, skyColor, upBlend) * ambientIntensity;
```

### 2. Progressive Merge Radius (VCM Thesis)
```cpp
// Equation from thesis: r_i = r_1 / sqrt(i - 1)
float mergeRadius(float r1, uint32_t iteration) {
    if (iteration <= 1) return r1;
    return r1 / sqrtf(static_cast<float>(iteration - 1));
}
```

### 3. Temporal Blend (TAGI)
```glsl
// In temporal_blend.comp
vec3 result = mix(history, current, blendFactor);  // blendFactor = 1/8
```

### 4. Normal Map Detail (No Re-Lighting)
```glsl
// In mega_geometry.frag
vec3 perturbedNormal = normalize(fragTBN * normalMapSample);
float detailOcc = 0.85 + 0.15 * max(dot(perturbedNormal, fragNormal), 0.0);
vec3 finalColor = albedo * vertexLighting * detailOcc;
```

---

## Vulkan Integration

### Compute Pipeline Setup
```cpp
// GI contributes ZERO draw calls
void GIRenderNode::record(VkCommandBuffer cmd) {
    // PASS 0: Light path tracing (only when dirty)
    if (m_giManager.isDirty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_lightTracePipeline);
        vkCmdDispatch(cmd, groups, 1, 1);
    }
    
    // PASS 1: Vertex lighting (TAGI: 1/8 tile per frame)
    auto tile = m_tagiScheduler.nextFrame(vertexCount);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_mergePipeline);
    vkCmdDispatch(cmd, (tile.tileSize + 255) / 256, 1, 1);
    
    // Barrier: compute write → vertex shader read
    VkBufferMemoryBarrier2 barrier{
        .srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
    };
}
```

### VMA Buffer Allocation
```cpp
// All buffers pre-allocated at startup, zero runtime allocation
void GIManager::allocateBuffers(VmaAllocator vma) {
    // Light vertex ring buffer: 65536 × 48 = ~3MB
    VkBufferCreateInfo bci{
        .size  = sizeof(LightVertex) * 65536,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    vmaCreateBuffer(vma, &bci, &aci, &m_lightVertBuffer, &m_lightVertAlloc, nullptr);
    
    // Vertex light buffer: 2M × 4 = 8MB
    // History buffer: 2M × 4 = 8MB
    // Total: ~19.4MB
}
```

---

## ADPF Thermal Throttling

```cpp
class ADPFGIThrottle {
    uint32_t tilesThisFrame(float thermalHeadroom) {
        if (thermalHeadroom < 0.1f) return 0;   // Emergency: skip GI
        if (thermalHeadroom < 0.3f) return 1;   // Warm: 1/8 tile
        if (thermalHeadroom < 0.7f) return 2;   // Normal: 2/8 tiles
        return 4;                                // Cool: 4/8 tiles
    }
};
```

---

## Performance Budget

### Draw Call Budget
| System | Draw Calls |
|--------|------------|
| GI Light Path Trace | 0 (compute) |
| GI Vertex Merge | 0 (compute) |
| GI Temporal Blend | 0 (compute) |
| **GI Total** | **0** |
| **Remaining for geometry** | **20** |

### VRAM Budget (2M vertices, 65K light vertices)
| Buffer | Size |
|--------|------|
| LightVertex ring buffer | 3.1 MB |
| Vertex light buffer (current) | 8.0 MB |
| Vertex light buffer (history) | 8.0 MB |
| Probe grid (2048 probes, Phase 4) | 0.25 MB |
| **Total GI VRAM** | **~19.4 MB** |
| **Lightmaps saved** | **4–64 MB** |

### Frame Time Budget (240 FPS = 4.16ms per frame)
| Pass | Budget | Notes |
|------|--------|-------|
| GI Compute (full refresh) | 0.3ms | Async compute queue |
| GI Compute (TAGI 1/8 tile) | 0.04ms | Per frame normal |
| Vertex shader (reads prelit) | Near-zero | Was lighting, now just read |
| Fragment shader savings | -0.4ms | vs fragment lighting |
| **Net GI cost at 240fps** | **~-0.36ms** | **(net gain)** |

---

## Migration Checklist

### Phase 1 (Days 1–4)
- [ ] Add `uint vertexLight` field to Vertex struct (4-byte aligned)
- [ ] Create `LightUBO` with 16-light capacity
- [ ] Implement `vertex_lighting_p1.comp`
- [ ] Simplify `mega_geometry.frag` (remove all light uniforms)
- [ ] Verify: FPS > 700 (was 560)
- [ ] Verify: Hemisphere ambient visible

### Phase 2 (Days 5–11)
- [ ] Implement `LightVertex` struct (48 bytes, oct-encoded)
- [ ] Implement `LightPathTracer::trace()` (CPU, single-bounce)
- [ ] Implement `SpatialHash` (zero-alloc)
- [ ] Implement `vertex_merge.comp`
- [ ] Wire `GIManager::markDirty()` to light changes
- [ ] Test: Move light → GI updates in ≤8 frames

### Phase 3 (Days 12–16)
- [ ] Implement `TAGIScheduler` (1/8 tile per frame)
- [ ] Implement `temporal_blend.comp`
- [ ] Add `ADPFGIThrottle` to ADPF hook
- [ ] Verify: GI overhead <0.1ms per frame
- [ ] Verify: No ghosting on fast-moving lights

### Phase 4 (Optional, 2 weeks)
- [ ] Define `ProbeGrid` layout and `IrradianceProbe` SH9
- [ ] Implement probe baking pass
- [ ] Add probe sampling to `mega_geometry.vert`
- [ ] Blend probe GI with vertex-merged GI
- [ ] Test: 10K instance scene quality

### Invariants to Verify Every Phase
- [ ] Total draw calls ≤20 (GI contributes 0)
- [ ] Zero new/malloc in hot path
- [ ] VRAM usage ≤20MB for GI
- [ ] 240 FPS maintained on target device

---

## Quick Reference: File Structure

```
core/rendering/gi/
├── LightVertex.h              // 48-byte struct, oct-encoding
├── LightPathTracer.h/.cpp     // CPU light tracing
├── SpatialHash.h/.cpp         // Zero-alloc spatial hash
├── GIManager.h/.cpp           // Main GI coordinator
├── TAGIScheduler.h            // Temporal accumulation
├── ADPFGIThrottle.h           // Thermal throttling
└── ProbeGrid.h/.cpp           // Phase 4 probe system

shaders/gi/
├── vertex_lighting_p1.comp    // Phase 1: Direct lighting
├── vertex_merge.comp          // Phase 2: Indirect lighting
├── temporal_blend.comp        // Phase 3: Temporal blend
└── probe_sample.glsl          // Phase 4: Probe sampling

shaders/
├── mega_geometry.vert         // Reads prelit vertex colors
└── mega_geometry.frag         // Albedo × lighting × detail
```

---

## Next Steps

1. **Start with Phase 1** (highest ROI, 3-4 days)
   - Immediate FPS gain: 560 → 720+
   - Zero draw call overhead
   - Foundation for all future phases

2. **Profile after each phase**
   - Use VkQueryPool timestamps
   - Verify frame time budget
   - Check VRAM usage

3. **Iterate based on results**
   - If Phase 1 hits targets → proceed to Phase 2
   - If thermal throttling needed → add ADPF early
   - If large scenes → consider Phase 4 probes

---

**Status:** Specification complete, ready for implementation  
**Priority:** High - replaces lightmaps, enables fully dynamic lighting  
**Estimated Total Effort:** 4-6 weeks (all phases)  
**Phase 1 Effort:** 3-4 days (immediate value)
