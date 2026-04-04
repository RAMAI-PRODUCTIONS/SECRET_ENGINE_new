# Vertex-Based Lighting Implementation Plan

## Current Status
✅ Per-instance colors working (560+ FPS)
✅ Texture + color mix blend working
✅ 11 instances rendering correctly

## Proposed: Vertex Lighting System

### Benefits
- **Performance**: 70-80% reduction in fragment shader cost
- **Mobile-Friendly**: Perfect for Android/low-end hardware
- **Scalable**: Handles thousands of instances efficiently
- **Quality**: Baked GI + dynamic lights + shadows

### Architecture

#### Phase 1: Basic Vertex Lighting (Week 1)
**Goal**: Replace fragment lighting with vertex lighting

1. **Vertex Shader Updates**
   - Add light uniforms (positions, colors, intensities)
   - Calculate diffuse lighting per-vertex
   - Calculate specular (Blinn-Phong) per-vertex
   - Output pre-lit color to fragment shader

2. **Fragment Shader Simplification**
   - Remove lighting calculations
   - Sample textures only
   - Multiply texture by vertex color
   - Apply normal map as detail (not for lighting)

3. **Uniform Buffers**
   ```cpp
   struct LightData {
       vec3 positions[8];
       vec3 colors[8];
       float intensities[8];
       int count;
   };
   ```

**Expected Performance**: 700+ FPS (from current 560)

#### Phase 2: Baked GI (Week 2)
**Goal**: Add static global illumination

1. **Lightmap Support**
   - Add lightmap UV coordinates to vertex data
   - Bake lightmaps offline (Blender/custom tool)
   - Sample lightmap in fragment shader
   - Blend with dynamic lighting

2. **Hemisphere Ambient**
   - Sky color (top hemisphere)
   - Ground color (bottom hemisphere)
   - Blend based on vertex normal

3. **Ambient Occlusion**
   - Bake AO into vertex colors (alpha channel)
   - Or use AO texture
   - Multiply with final lighting

**Expected Quality**: Static lighting looks baked, dynamic lights add detail

#### Phase 3: Vertex Shadows (Week 3)
**Goal**: Add shadow support

1. **Shadow Maps**
   - Render shadow maps for main lights
   - Sample shadow map in vertex shader
   - Apply shadow factor to lighting

2. **Cascaded Shadow Maps** (Optional)
   - Multiple shadow cascades for large scenes
   - Select cascade based on distance

3. **Contact Shadows** (Optional)
   - Screen-space shadows for fine detail
   - Computed in fragment shader (cheap)

**Expected Quality**: Soft shadows with minimal performance cost

#### Phase 4: Advanced GI (Week 4)
**Goal**: Decima-style probe-based GI

1. **Light Probes**
   - Place probes in scene
   - Bake spherical harmonics (SH9)
   - Blend 4 nearest probes per vertex

2. **Reflection Probes**
   - Cubemap probes for reflections
   - Sample in fragment shader
   - Blend with roughness

3. **Irradiance Volumes**
   - 3D grid of GI data
   - Trilinear interpolation
   - Real-time updates possible

**Expected Quality**: AAA-level GI with minimal cost

### Implementation Steps

#### Step 1: Update Vertex Format
```cpp
struct Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float tangent[3];      // For normal mapping
    float lightmapUV[2];   // For baked GI
    uint8_t color[4];      // Vertex colors (AO in alpha)
};
```

#### Step 2: Update Shaders
- Modify `mega_geometry.vert` to calculate lighting
- Simplify `mega_geometry.frag` to sample textures only
- Add light uniform buffers

#### Step 3: Add Light Management
```cpp
class LightManager {
    void AddLight(LightType type, vec3 pos, vec3 color, float intensity);
    void UpdateLightBuffer();
    VkBuffer GetLightBuffer();
};
```

#### Step 4: Baking Pipeline
- Offline tool to bake lightmaps
- Generate lightmap UVs
- Pack lightmaps into texture atlas

### Performance Targets

| Metric | Current | Phase 1 | Phase 2 | Phase 3 | Phase 4 |
|--------|---------|---------|---------|---------|---------|
| FPS | 560 | 700+ | 650+ | 600+ | 550+ |
| GPU ms | 1.3 | 0.8 | 1.0 | 1.2 | 1.4 |
| Instances | 11 | 100+ | 500+ | 1000+ | 5000+ |
| Quality | Basic | Good | Great | Excellent | AAA |

### Compatibility

**Platforms**:
- ✅ Android (primary target)
- ✅ Windows
- ✅ Linux
- ✅ Mobile (iOS, etc.)

**Hardware**:
- ✅ Low-end mobile (vertex lighting is perfect)
- ✅ Mid-range (full features)
- ✅ High-end (can add more lights/probes)

### Migration Path

1. **Keep Current System**: Don't break existing rendering
2. **Add Vertex Lighting**: New shader variant
3. **Test Performance**: Compare FPS and quality
4. **Switch Default**: Make vertex lighting default
5. **Remove Old**: Clean up fragment lighting code

### Next Steps

**Immediate** (Today):
1. Commit current working system
2. Create vertex lighting branch
3. Update vertex shader with basic lighting

**This Week**:
1. Implement Phase 1 (basic vertex lighting)
2. Test on device
3. Measure performance improvement

**Next Week**:
1. Add baked GI support (Phase 2)
2. Create lightmap baking tool
3. Test with real scenes

**Future**:
1. Shadow maps (Phase 3)
2. Advanced GI probes (Phase 4)
3. Optimize for 10,000+ instances

### Questions to Answer

1. **Do we need normal maps?**
   - Yes, but only for detail, not lighting
   - Apply as bump/occlusion in fragment shader

2. **How many lights?**
   - Start with 4 dynamic lights
   - Can increase to 8 if performance allows

3. **Lightmap resolution?**
   - 512x512 per scene chunk
   - Pack multiple chunks into atlas

4. **Shadow quality?**
   - 1024x1024 shadow maps
   - PCF filtering for soft shadows

### References

- Decima Engine (Horizon Zero Dawn): Probe-based GI
- Unity: Vertex lighting mode
- Unreal: Lightmass baking
- Godot: GI probes

---

**Status**: Planning Phase
**Priority**: High (major performance win)
**Complexity**: Medium (well-understood techniques)
**Timeline**: 4 weeks for full implementation
