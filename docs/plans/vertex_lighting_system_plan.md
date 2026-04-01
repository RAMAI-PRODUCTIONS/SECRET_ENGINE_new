# Vertex-Based Dynamic Lighting System Implementation Plan

## Overview
Complete implementation of a fully dynamic, vertex-based lighting system with **no lightmaps** - all lighting computed at vertices and stored in vertex colors. Provides Decima-like quality with GameCube-era performance.

## Core Architecture

### Key Features
- **All lights dynamic** - Point, directional, spot lights with full animation support
- **Per-vertex lighting** - All lighting data stored in vertices, super fast
- **Dynamic GI** - Real-time global illumination using probe system
- **No lightmaps** - Zero texture memory for GI, no UV2 required
- **Normal map detail** - Adds surface detail without recomputing lighting
- **Per-instance tint** - Thousands of unique colors without extra draw calls
- **Hemisphere GI** - Realistic ambient occlusion without baking
- **C++26 parallel processing** - Multi-threaded lighting computation

### Performance Targets
- 1000+ instances with dynamic lighting at 60+ FPS
- Hundreds of dynamic lights with spatial culling
- Parallel vertex lighting computation
- Cache-optimized data structures

## Implementation Components

### 1. Core Data Structures (LightingSystem.h)

#### Light Types
```cpp
enum class LightKind : uint8_t {
    Point = 0,
    Directional = 1,
    Spot = 2,
    Area = 3
};
```

#### PointLight Structure
- Position, radius, color, intensity
- Attenuation parameters (linear, quadratic)
- Shadow casting flags
- GI contribution flags
- Distance-based attenuation calculation

#### DirectionalLight Structure
- Direction, intensity, color
- Angular diameter (for sun simulation)
- Shadow casting support
- GI contribution

#### SpotLight Structure
- Position, direction, radius
- Inner/outer cutoff angles
- Color, intensity
- Spot factor calculation
- Shadow casting

#### GIProbe Structure
- Position, radius
- Incoming radiance (6 cube map faces)
- Spherical harmonics coefficients (9 SH terms)
- Update timing and dirty flags
- Dynamic probe update from all light sources

#### DynamicInstance Structure
- Transform matrix
- Per-instance tint
- World position and bounding radius
- Vertex data (positions, normals, colors)
- Lighting update flags
- Vertex/index buffer offsets

### 2. LightManager Class

#### Core Responsibilities
- Manage all dynamic lights (add/remove/update)
- Manage instances with per-vertex lighting
- Update GI probes dynamically
- Parallel lighting computation
- GPU buffer data generation

#### Key Methods
- `addPointLight()` / `addDirectionalLight()` / `addSpotLight()`
- `removePointLight()` / `updateInstanceTransform()`
- `updateGIProbes(deltaTime)` - Dynamic probe updates
- `updateAllInstanceLighting(cameraPosition)` - Main parallel update
- `getVertexBufferData()` - Pack data for GPU upload

#### Lighting Computation
- `computeInstanceLighting()` - Per-instance parallel processing
- `calculatePointLight()` - Blinn-Phong point light
- `calculateDirectionalLight()` - Directional light with specular
- `calculateSpotLight()` - Spot light with cone attenuation
- `sampleGIProbes()` - Spherical harmonics GI sampling

### 3. GPU Shaders

#### Vertex Shader (VertexLighting.vert)
**Inputs:**
- Position, normal, texcoord
- Pre-computed vertex color (from CPU lighting)
- Per-instance transform matrix
- Per-instance tint

**Processing:**
- Transform to world space
- Pass pre-lit vertex colors to fragment shader
- Multiply by instance tint for variation
- No lighting computation (already done on CPU!)

**Outputs:**
- World position and normal
- Texture coordinates
- Pre-lit vertex color

#### Fragment Shader (VertexLighting.frag)
**Inputs:**
- Pre-lit vertex color
- Texture coordinates
- World position and normal

**Processing:**
- Sample albedo, normal map, roughness textures
- Combine textures with pre-lit vertex color
- Add normal map detail as micro-occlusion
- Simple rim lighting based on view angle
- Tone mapping and gamma correction

**No lighting computation** - just texture sampling and detail enhancement!

### 4. Advanced Features

#### Hemisphere GI
```cpp
vec3 calculateHemisphereGI(vec3 normal, vec3 worldPos) {
    float upFactor = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 hemisphereColor = mix(groundColor, skyColor, upFactor);
    float heightBounce = clamp((worldPos.y + 10.0) / 20.0, 0.2, 0.8);
    hemisphereColor += vec3(heightBounce * 0.1);
    return hemisphereColor * hemisphereIntensity;
}
```

#### Dynamic GI Probes
- 6-face cube map radiance accumulation
- Spherical harmonics for smooth interpolation
- Distance-based probe blending
- Automatic updates based on light changes
- Camera-relative update prioritization

#### Light Culling System
```cpp
class LightCuller {
    static std::vector<size_t> cullPointLights(
        const std::vector<PointLight>& lights,
        const glm::vec3& cameraPos,
        const glm::mat4& viewProj
    );
};
```
- Frustum vs sphere testing
- Distance-based culling
- Parallel processing with std::execution::par

#### Instance LOD System
```cpp
class InstanceLOD {
    struct LODLevel {
        float maxDistance;
        uint32_t vertexCount;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
    };
    void updateLOD(DynamicInstance& instance, float distance);
};
```
- Distance-based vertex count reduction
- Automatic LOD switching
- Lighting update on LOD change

### 5. Usage Example

#### Initialization
```cpp
// Setup sun
DirectionalLight sun;
sun.direction = glm::normalize(glm::vec3(1.0f, -2.0f, 0.5f));
sun.color = glm::vec3(1.0f, 0.95f, 0.85f);
sun.intensity = 1.2f;
lightManager.addDirectionalLight(std::move(sun));

// Add dynamic point lights (50 torches)
for (int i = 0; i < 50; ++i) {
    PointLight torch;
    torch.position = randomPosition();
    torch.color = glm::vec3(1.0f, 0.5f, 0.2f);
    torch.intensity = 1.5f;
    torch.radius = 8.0f;
    lightManager.addPointLight(std::move(torch));
}

// Setup GI probes (grid)
for (int x = -3; x <= 3; ++x) {
    for (int z = -3; z <= 3; ++z) {
        GIProbe probe;
        probe.position = glm::vec3(x * 10.0f, 5.0f, z * 10.0f);
        probe.radius = 15.0f;
        // Add to system
    }
}
```

#### Per-Frame Update
```cpp
void update(float deltaTime) {
    // Animate lights (day/night cycle, moving torches)
    updateDynamicLights(time);
    
    // Update GI probes
    lightManager.updateGIProbes(deltaTime);
    
    // Recompute all instance lighting (PARALLEL!)
    glm::vec3 cameraPosition = getCameraPosition();
    lightManager.updateAllInstanceLighting(cameraPosition);
    
    // Get updated vertex buffers for GPU
    auto vertexData = lightManager.getVertexBufferData();
    
    // Upload to GPU and render
    uploadAndRender(vertexData);
}
```

#### Dynamic Light Animation
```cpp
void updateDynamicLights(float time) {
    // Animate point lights (moving torches)
    for (size_t i = 0; i < pointLights.size(); ++i) {
        pointLights[i].position.x = 10.0f * sin(time * 2.0f + i);
        pointLights[i].position.z = 10.0f * cos(time * 1.5f + i);
        pointLights[i].intensity = 1.0f + sin(time * 3.0f + i) * 0.5f;
    }
    
    // Day/night cycle for sun
    float sunAngle = time * 0.2f;
    dirLights[0].direction = glm::normalize(glm::vec3(
        cos(sunAngle),
        sin(sunAngle) * 0.8f,
        sin(sunAngle * 0.5f)
    ));
    dirLights[0].intensity = 0.5f + (sin(sunAngle) * 0.5f + 0.5f) * 1.2f;
}
```

## Art Pipeline Integration

### Vertex Color Baking (DCC Tools)
1. **Ambient Occlusion** → Bake to vertex colors (red channel)
2. **Bent Normals** → For directional GI (green/blue channels)
3. **Emissive Hints** → Where light should bounce (alpha channel)

### Blender Example
```python
import bpy

# Bake AO to vertex colors
bpy.ops.object.mode_set(mode='VERTEX_PAINT')
bpy.ops.paint.vertex_color_bake()
```

### Export Requirements
- Vertex positions, normals, tangents, bitangents
- Vertex colors (RGBA)
- Texture coordinates (UV)
- Per-instance transforms and tints

## Performance Optimizations

### Parallel Processing
- C++26 `std::execution::par` for all lighting computations
- Per-instance parallel vertex processing
- Light culling in parallel
- GI probe updates in parallel

### Spatial Culling
- Frustum culling for lights
- Distance-based light radius culling
- Per-vertex light contribution culling
- Camera-relative probe updates

### Memory Optimization
- Aligned data structures (16/32/64 byte alignment)
- Cache-friendly memory layout
- Minimal GPU uploads (only vertex colors change)
- LOD system for distant instances

### SIMD Opportunities
- Vectorized lighting calculations
- Batch vertex transformations
- Parallel dot products and cross products

## Integration Steps

### Phase 1: Core Infrastructure
1. Implement light data structures (PointLight, DirectionalLight, SpotLight)
2. Create LightManager class with basic add/remove functionality
3. Implement DynamicInstance structure with vertex data
4. Setup parallel processing infrastructure

### Phase 2: Lighting Computation
1. Implement per-vertex lighting calculations
2. Add Blinn-Phong shading model
3. Implement hemisphere GI
4. Add light attenuation and falloff

### Phase 3: Dynamic GI System
1. Implement GIProbe structure
2. Add spherical harmonics computation
3. Implement probe update system
4. Add probe blending and interpolation

### Phase 4: GPU Integration
1. Create vertex shader with pre-lit colors
2. Create fragment shader with texture detail
3. Implement GPU buffer upload system
4. Add per-instance rendering

### Phase 5: Optimizations
1. Implement light culling system
2. Add instance LOD system
3. Optimize parallel processing
4. Profile and tune performance

### Phase 6: Advanced Features
1. Add day/night cycle support
2. Implement weather effects (fog, rain)
3. Add vertex animation (wind, grass)
4. Implement shadow casting (optional)

## Testing Strategy

### Unit Tests
- Light attenuation calculations
- Spherical harmonics computation
- Probe blending weights
- Frustum culling accuracy

### Performance Tests
- 1000 instances with 100 lights
- Parallel processing scalability
- Memory usage profiling
- Frame time analysis

### Visual Tests
- Compare with reference images
- Verify GI probe interpolation
- Check normal map detail
- Validate day/night transitions

## Future Enhancements

### Potential Additions
1. **Dynamic time-of-day** lighting transitions
2. **Weather effects** (fog, rain) that respect vertex lighting
3. **Vertex animation** with wind/grass movement
4. **Shadow casting** from dynamic lights (optional)
5. **Light probes** for character lighting
6. **Volumetric fog** using vertex-based density
7. **Emissive materials** contributing to GI
8. **Light baking** for static geometry (hybrid approach)

### Advanced GI
- Voxel-based GI for more accurate bounce lighting
- Light propagation volumes (LPV)
- Screen-space GI for additional detail
- Temporal accumulation for smoother GI

## References

### Technical Resources
- "Real-Time Rendering" (4th Edition) - Lighting chapters
- "GPU Gems" series - Vertex lighting techniques
- Decima Engine presentations - Horizon Zero Dawn lighting
- "Practical Real-Time Strategies for Accurate Indirect Occlusion"

### Implementation Examples
- Unity's vertex lighting system
- Unreal Engine's simple lighting mode
- Source Engine's vertex lighting
- Nintendo's vertex lighting in Zelda: BotW

## Notes

### Design Philosophy
- **CPU does lighting** - GPU just samples textures
- **Vertex colors are king** - All lighting stored in vertices
- **No lightmaps** - Zero texture memory overhead
- **Fully dynamic** - Everything can move and change
- **Parallel everything** - Modern C++26 concurrency

### Trade-offs
- **CPU overhead** - More CPU work per frame
- **Vertex density** - Need enough vertices for smooth lighting
- **Memory** - Vertex colors add 16 bytes per vertex
- **Flexibility** - Can't do per-pixel lighting effects

### When to Use
- Open world games with many instances
- Mobile/low-end platforms
- Stylized art styles (Zelda, Genshin Impact)
- Games with fully dynamic lighting
- When texture memory is limited

### When NOT to Use
- Photorealistic games requiring per-pixel lighting
- Indoor scenes with complex lighting
- Games with static lighting (use lightmaps instead)
- When CPU is bottleneck

---

**Status:** Planned for future implementation
**Priority:** Medium
**Estimated Effort:** 3-4 weeks
**Dependencies:** Modern C++26 compiler, GLM, Vulkan renderer
