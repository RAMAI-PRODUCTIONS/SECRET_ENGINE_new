# Forward+ Lighting System Implementation Guide

## Overview

This document outlines the implementation of a Forward+ (Tiled Forward) rendering system for SecretEngine, inspired by the [Forward+ Renderer by Bradley Crusco and Megan Moore](https://github.com/bcrusco/Forward-Plus-Renderer) and the paper "Forward+: Bringing Deferred Rendering to the Next Level" by Takahiro Harada, Jay McKee, and Jason C. Yang.

**Performance Benefits:**
- 1024 lights @ 1080p: 89.867 FPS (Forward+) vs 1.7 FPS (Traditional Forward)
- ~53x performance improvement for many-light scenarios
- Scales efficiently with light count

## Core Concept

Forward+ combines the benefits of forward and deferred rendering:
- **Forward Rendering**: Direct lighting calculation, supports MSAA, transparent objects
- **Deferred Rendering**: Efficient handling of many lights through culling
- **Forward+**: Adds a light culling pre-pass to forward rendering using compute shaders

## Three-Stage Pipeline

### Stage 1: Depth Prepass

**Purpose:** Generate a depth buffer for the entire scene from the camera's perspective.

**Implementation Steps:**

1. **Render Pass Setup**
   - Create depth-only framebuffer
   - Disable color writes
   - Enable depth testing and writing

2. **Depth Linearization**
   ```glsl
   // Linearize depth from projection space
   float linearDepth = (2.0 * near * far) / (far + near - depth * (far - near));
   ```

3. **Vulkan Implementation**
   - Use VK_FORMAT_D32_SFLOAT for depth buffer
   - Create separate render pass for depth prepass
   - Render all opaque geometry

**Output:** Depth buffer texture accessible by compute shader

---

### Stage 2: Light Culling (Compute Shader)

**Purpose:** Determine which lights affect which screen tiles using GPU parallelism.

#### Tile Configuration

**Recommended Settings:**
- Tile size: 16x16 pixels (256 threads per workgroup)
- Alternative: 8x8 pixels (64 threads) - more accurate but slower
- Maximum lights: 1024 (configurable)

**Calculation:**
```cpp
uint32_t numTilesX = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
uint32_t numTilesY = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;
uint32_t totalTiles = numTilesX * numTilesY;
```

#### Compute Shader Algorithm (Gather Approach)

**Step 1: Find Tile Min/Max Depth**
```glsl
layout(local_size_x = 16, local_size_y = 16) in;

shared float minDepth;
shared float maxDepth;

void main() {
    // Initialize shared memory
    if (gl_LocalInvocationIndex == 0) {
        minDepth = 1.0;
        maxDepth = 0.0;
    }
    barrier();
    
    // Each thread reads its pixel's depth
    ivec2 pixelPos = ivec2(gl_GlobalInvocationID.xy);
    float depth = texelFetch(depthBuffer, pixelPos, 0).r;
    depth = linearizeDepth(depth);
    
    // Atomic min/max operations
    atomicMin(minDepth, depth);
    atomicMax(maxDepth, depth);
    barrier();
}
```

**Step 2: Calculate Tile Frustum**
```glsl
// One thread calculates frustum planes for the tile
if (gl_LocalInvocationIndex == 0) {
    vec2 tileScale = vec2(screenSize) / float(TILE_SIZE);
    vec2 tileBias = tileScale - vec2(gl_WorkGroupID.xy);
    
    // Calculate frustum planes in view space
    vec4 c1 = vec4(projection[0][0] * tileScale.x, 0.0, tileBias.x, 0.0);
    vec4 c2 = vec4(0.0, -projection[1][1] * tileScale.y, tileBias.y, 0.0);
    vec4 c4 = vec4(0.0, 0.0, 1.0, 0.0);
    
    // Four side planes + near/far
    frustumPlanes[0] = c4 - c1;  // Left
    frustumPlanes[1] = c4 + c1;  // Right
    frustumPlanes[2] = c4 - c2;  // Bottom
    frustumPlanes[3] = c4 + c2;  // Top
    frustumPlanes[4] = vec4(0.0, 0.0, -1.0, -minDepth);  // Near
    frustumPlanes[5] = vec4(0.0, 0.0, 1.0, maxDepth);    // Far
}
barrier();
```

**Step 3: Cull Lights Against Frustum**
```glsl
shared uint visibleLightCount;
shared uint visibleLightIndices[MAX_LIGHTS_PER_TILE];

// Parallelize over lights (256 threads can test 256 lights simultaneously)
for (uint i = 0; i < numLights; i += 256) {
    uint lightIndex = i + gl_LocalInvocationIndex;
    
    if (lightIndex < numLights) {
        Light light = lights[lightIndex];
        
        // Transform light to view space
        vec3 lightPosView = (view * vec4(light.position, 1.0)).xyz;
        
        // Test sphere against all 6 frustum planes
        bool inFrustum = true;
        for (int j = 0; j < 6; j++) {
            float dist = dot(frustumPlanes[j].xyz, lightPosView) + frustumPlanes[j].w;
            if (dist < -light.radius) {
                inFrustum = false;
                break;
            }
        }
        
        // Add to visible list
        if (inFrustum) {
            uint index = atomicAdd(visibleLightCount, 1);
            if (index < MAX_LIGHTS_PER_TILE) {
                visibleLightIndices[index] = lightIndex;
            }
        }
    }
    barrier();
}
```

**Step 4: Write Results to SSBO**
```glsl
// Write visible light indices to global buffer
if (gl_LocalInvocationIndex == 0) {
    uint tileIndex = gl_WorkGroupID.y * numTilesX + gl_WorkGroupID.x;
    uint offset = tileIndex * (MAX_LIGHTS_PER_TILE + 1);
    
    // Store count first
    lightGrid[offset] = visibleLightCount;
    
    // Store indices
    for (uint i = 0; i < visibleLightCount; i++) {
        lightGrid[offset + 1 + i] = visibleLightIndices[i];
    }
}
```

#### Buffer Layout

**Light Grid SSBO:**
```cpp
struct LightGrid {
    uint32_t count;                      // Number of lights in tile
    uint32_t indices[MAX_LIGHTS_PER_TILE]; // Light indices
};

// Total size: numTiles * (1 + MAX_LIGHTS_PER_TILE) * sizeof(uint32_t)
```

---

### Stage 3: Light Accumulation & Final Shading

**Purpose:** Render scene using only the culled lights per tile.

#### Fragment Shader Implementation

```glsl
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;

// Textures
layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D specularMap;
layout(binding = 2) uniform sampler2D normalMap;

// Light grid SSBO
layout(std430, binding = 3) readonly buffer LightGridSSBO {
    uint lightGrid[];
};

// Lights SSBO
layout(std430, binding = 4) readonly buffer LightsSSBO {
    Light lights[];
};

void main() {
    // Determine which tile this fragment belongs to
    ivec2 tileID = ivec2(gl_FragCoord.xy) / ivec2(TILE_SIZE, TILE_SIZE);
    uint tileIndex = tileID.y * numTilesX + tileID.x;
    uint offset = tileIndex * (MAX_LIGHTS_PER_TILE + 1);
    
    // Get light count for this tile
    uint lightCount = lightGrid[offset];
    
    // Sample textures
    vec3 albedo = texture(diffuseMap, fragTexCoord).rgb;
    vec3 specular = texture(specularMap, fragTexCoord).rgb;
    vec3 normalTS = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
    
    // Transform normal to world space (tangent space normal mapping)
    mat3 TBN = mat3(normalize(fragTangent), 
                    normalize(fragBitangent), 
                    normalize(fragNormal));
    vec3 normal = normalize(TBN * normalTS);
    
    // Accumulate lighting
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);
    
    vec3 viewDir = normalize(cameraPos - fragPos);
    
    // Loop through visible lights only
    for (uint i = 0; i < lightCount; i++) {
        uint lightIndex = lightGrid[offset + 1 + i];
        Light light = lights[lightIndex];
        
        // Calculate lighting (Blinn-Phong)
        vec3 lightDir = normalize(light.position - fragPos);
        float distance = length(light.position - fragPos);
        
        // Attenuation
        float attenuation = 1.0 / (1.0 + light.linear * distance + 
                                   light.quadratic * distance * distance);
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * light.color * attenuation;
        
        // Specular (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        vec3 specularContrib = spec * light.color * attenuation;
        
        totalDiffuse += diffuse;
        totalSpecular += specularContrib;
    }
    
    // Combine lighting
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + totalDiffuse * albedo + totalSpecular * specular;
    
    // HDR tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}
```

---

## Additional Features

### 1. Tangent Space Normal Mapping

**Benefits:**
- Better visual fidelity without heavy performance cost
- Normal maps work regardless of object orientation

**Implementation:**
```cpp
// Calculate tangent and bitangent during mesh loading
for (each triangle) {
    vec3 edge1 = pos1 - pos0;
    vec3 edge2 = pos2 - pos0;
    vec2 deltaUV1 = uv1 - uv0;
    vec2 deltaUV2 = uv2 - uv0;
    
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
    
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    
    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
}
```

### 2. HDR with Reinhard Tone Mapping

**Implementation:**

1. **Render to floating-point framebuffer**
   ```cpp
   VkFormat hdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
   ```

2. **Tone mapping pass**
   ```glsl
   vec3 hdrColor = texture(hdrBuffer, texCoord).rgb;
   vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
   outColor = vec4(mapped, 1.0);
   ```

### 3. Debug Visualizations

**Depth Buffer Debug:**
```glsl
float depth = texture(depthBuffer, texCoord).r;
outColor = vec4(vec3(depth), 1.0);
```

**Light Count Debug:**
```glsl
uint lightCount = lightGrid[tileIndex * (MAX_LIGHTS_PER_TILE + 1)];
float intensity = float(lightCount) / float(MAX_LIGHTS_PER_TILE);
outColor = vec4(vec3(intensity), 1.0);
```

---

## Integration with SecretEngine

### 1. Modify ILightingSystem Interface

```cpp
// core/include/SecretEngine/ILightingSystem.h
class ILightingSystem {
public:
    virtual ~ILightingSystem() = default;
    
    // Forward+ specific methods
    virtual void SetTileSize(uint32_t size) = 0;
    virtual void SetMaxLightsPerTile(uint32_t maxLights) = 0;
    virtual void EnableDebugVisualization(DebugMode mode) = 0;
    
    // Existing methods
    virtual void AddLight(const LightComponent& light) = 0;
    virtual void RemoveLight(EntityID entity) = 0;
    virtual void Update(float deltaTime) = 0;
};

enum class DebugMode {
    None,
    DepthBuffer,
    LightCount,
    Frustums
};
```

### 2. Create Forward+ Lighting Plugin

```cpp
// plugins/ForwardPlusLighting/src/ForwardPlusLightingPlugin.cpp
class ForwardPlusLightingSystem : public ILightingSystem {
private:
    // Vulkan resources
    VkBuffer lightBuffer;
    VkBuffer lightGridBuffer;
    VkDescriptorSet computeDescriptorSet;
    VkPipeline lightCullingPipeline;
    
    // Configuration
    uint32_t tileSize = 16;
    uint32_t maxLightsPerTile = 256;
    
    // Render passes
    VkRenderPass depthPrepass;
    VkRenderPass finalPass;
    
public:
    void Initialize(IRenderer* renderer) override;
    void PerformDepthPrepass(VkCommandBuffer cmd);
    void PerformLightCulling(VkCommandBuffer cmd);
    void RenderFinal(VkCommandBuffer cmd);
};
```

### 3. Shader Integration

Create shader directory structure:
```
Assets/shaders/forward_plus/
├── depth_prepass.vert
├── depth_prepass.frag
├── light_culling.comp
├── final_shading.vert
└── final_shading.frag
```

### 4. Performance Considerations

**Tile Size Selection:**
- 16x16: Best for most scenarios (89.86 FPS with 1024 lights)
- 8x8: More accurate culling but slower (25.7 FPS with 1024 lights)
- 32x32: Faster culling but less accurate

**Light Radius Impact:**
- Radius 30: Efficient, few lights per tile
- Radius 50: Less efficient, many lights per tile
- Consider dynamic LOD for light radius based on distance

**Memory Usage:**
```cpp
// Per-frame memory
size_t lightGridSize = numTiles * (1 + maxLightsPerTile) * sizeof(uint32_t);
// Example: 1920x1080, 16x16 tiles, 256 max lights
// = (120 * 68) * (1 + 256) * 4 = 8,366,080 bytes (~8 MB)
```

---

## Implementation Roadmap

### Phase 1: Foundation (Week 1-2)
- [ ] Implement depth prepass render pass
- [ ] Create compute shader infrastructure
- [ ] Set up SSBO for light grid

### Phase 2: Light Culling (Week 3-4)
- [ ] Implement tile frustum calculation
- [ ] Implement light-frustum intersection tests
- [ ] Optimize atomic operations

### Phase 3: Final Shading (Week 5-6)
- [ ] Implement tile-based light lookup
- [ ] Integrate with existing material system
- [ ] Add Blinn-Phong lighting model

### Phase 4: Advanced Features (Week 7-8)
- [ ] Tangent space normal mapping
- [ ] HDR with tone mapping
- [ ] Debug visualizations

### Phase 5: Optimization (Week 9-10)
- [ ] Profile and optimize compute shader
- [ ] Tune tile size for target hardware
- [ ] Implement light LOD system

---

## References

1. [Forward+: Bringing Deferred Rendering to the Next Level](https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf)
   - Takahiro Harada, Jay McKee, Jason C. Yang
   
2. [Forward+ Renderer Implementation](https://github.com/bcrusco/Forward-Plus-Renderer)
   - Bradley Crusco, Megan Moore
   
3. [DirectX 11 Rendering in Battlefield 3](http://www.dice.se/news/directx-11-rendering-battlefield-3/)
   - Johan Andersson

4. [Vulkan Compute Shaders](https://www.khronos.org/opengl/wiki/Compute_Shader)

---

## Performance Targets

**Target Metrics (1920x1080):**
- 100+ lights: 60+ FPS
- 500 lights: 60+ FPS
- 1024 lights: 45+ FPS

**Mobile Targets (1280x720):**
- 50 lights: 60 FPS
- 100 lights: 30+ FPS

---

*Document created: 2026-04-04*
*Based on Forward+ Renderer by Bradley Crusco and Megan Moore*
