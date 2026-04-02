# VCM Analysis: Vertex Lighting & GI Techniques for Our Engine

## Document Source
**Title:** Global Illumination with GPU Vertex Connection and Merging  
**Author:** João Monteiro  
**Institution:** Instituto Superior Técnico, Lisboa, Portugal  
**Date:** October 2020

## Executive Summary
This thesis explores GPU-accelerated vertex-based global illumination using the Vertex Connection and Merging (VCM) algorithm. Key insights for our engine implementation.

## Core Concepts Relevant to Our Engine

### 1. Vertex-Based Lighting Architecture

#### Light Vertices
- **Definition:** Intersection points where light rays hit geometry
- **Storage:** Position, throughput, contribution weights
- **Purpose:** Reusable lighting data (similar to photon mapping)
- **Key Insight:** Store lighting at vertices, not in textures (no lightmaps!)

```cpp
struct LightVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 throughput;      // Accumulated light energy
    float connectionWeight;     // For MIS
    float mergingWeight;        // For MIS
    uint32_t pathLength;
};
```

#### Camera Vertices
- **Definition:** Intersection points from camera rays
- **Not stored:** Computed on-the-fly per frame
- **Purpose:** Connect/merge with light vertices for final color



### 2. Two-Stage Rendering Process

#### Stage 1: Light Path Tracing
```cpp
// Pseudocode from thesis
for each pixel:
    lightVertex = TraceLightRay(pixel)
    while lightVertex is valid:
        if lightVertex is not specular:
            StoreLightVertex(lightVertex)
            ConnectToCamera(lightVertex)  // Direct contribution
        lightVertex = ContinueRandomWalk(lightVertex)
```

**Key Points:**
- Trace rays FROM light sources
- Store intersection data at vertices
- Connect directly to camera for immediate contribution
- Build spatial data structure for merging

#### Stage 2: Camera Path Tracing + Vertex Operations
```cpp
for each pixel:
    cameraVertex = TraceCameraRay(pixel)
    while cameraVertex is valid:
        // 1. Direct light sampling
        if cameraVertex hits emissive:
            Accumulate(cameraVertex)
        
        // 2. Vertex Connection (BDPT)
        for each lightVertex in same path:
            Accumulate(Connect(cameraVertex, lightVertex))
        
        // 3. Vertex Merging (Photon Mapping)
        for each lightVertex within radius:
            Accumulate(Merge(cameraVertex, lightVertex))
        
        cameraVertex = ContinueRandomWalk(cameraVertex)
```

**Key Points:**
- Trace rays FROM camera
- Connect to stored light vertices (BDPT technique)
- Merge with nearby light vertices (PM technique)
- Use Multiple Importance Sampling (MIS) to weight contributions

### 3. Vertex Merging with Adaptive Radius

#### Progressive Radius Reduction
```cpp
// From thesis equation (1)
float calculateMergeRadius(float initialRadius, float alpha, int iteration) {
    return initialRadius / sqrt(iteration - 1);
}

// Example usage
float r1 = 0.1f;  // Initial radius
float alpha = 0.75f;  // Reduction constant
for (int i = 1; i <= iterations; ++i) {
    float ri = r1 / sqrt(i - 1);
    // Use ri for vertex merging this iteration
}
```

**Why This Matters:**
- **Early iterations:** Large radius = fast convergence, some blur
- **Later iterations:** Small radius = precise merging, less blur
- **Result:** Progressive refinement without artifacts

### 4. No Lightmaps - All Dynamic

#### Traditional Approach (What We're Avoiding)
```
❌ Bake lighting → UV2 → Lightmap textures → Memory overhead
❌ Static lighting only
❌ Cannot handle dynamic lights
❌ Requires rebaking for changes
```

#### VCM Approach (What We Want)
```
✅ Compute lighting at vertices per frame
✅ Store in vertex colors (4 bytes per vertex)
✅ Fully dynamic - lights can move
✅ No texture memory overhead
✅ Works with normal maps for detail
```

### 5. Multiple Importance Sampling (MIS)

#### Purpose
Combine different sampling techniques optimally to reduce variance (noise).

#### Weight Calculation
```cpp
float powerHeuristic(float pdf_a, float pdf_b, int beta = 2) {
    float a = pow(pdf_a, beta);
    float b = pow(pdf_b, beta);
    return a / (a + b);
}

// Usage in VCM
vec3 finalColor = vec3(0.0);
for each technique:
    float weight = powerHeuristic(technique.pdf, otherTechniques.pdf);
    finalColor += weight * technique.contribution / technique.pdf;
```

**Techniques Combined:**
1. **Unidirectional Sampling (US):** Direct camera-to-light
2. **Vertex Connection (VC):** BDPT connections
3. **Vertex Merging (VM):** Photon mapping merges

### 6. GPU Implementation Insights

#### Memory Hierarchy (From Thesis)
```
Fastest → Slowest:
1. Registers (per-thread, ~32KB)
2. Shared Memory (per-SM, 48-96KB)
3. L1/L2 Cache (automatic)
4. Global Memory (slowest, 8-24GB)
```

#### Optimization Strategies
```cpp
// Store frequently accessed data in shared memory
__shared__ Light lights[MAX_LIGHTS];
__shared__ Material materials[MAX_MATERIALS];

// Each thread processes one pixel
int pixelIndex = blockIdx.x * blockDim.x + threadIdx.x;

// Minimize global memory writes
// Use per-iteration frame buffers to reduce collisions
```

#### Performance Bottlenecks Identified
1. **Memory divergence:** Rays bounce in random directions
2. **Cache misses:** Adjacent pixels access distant memory
3. **Thread divergence:** Different path lengths per pixel
4. **Global memory writes:** Frame buffer updates

### 7. Practical Implementation for Our Engine

#### Simplified VCM for Real-Time

**Option A: Pre-compute Light Vertices (Hybrid)**
```cpp
class SimplifiedVCM {
    // Pre-compute light vertices once per light change
    std::vector<LightVertex> m_lightVertices;
    
    void updateLightVertices() {
        m_lightVertices.clear();
        
        // Trace light paths (can be done on GPU)
        for (auto& light : m_lights) {
            traceLightPaths(light, m_lightVertices);
        }
        
        // Build spatial hash for fast queries
        m_spatialHash.build(m_lightVertices);
    }
    
    // Per-frame: Compute vertex colors
    void computeVertexColors(Mesh& mesh) {
        for (auto& vertex : mesh.vertices) {
            vec3 color = vec3(0.0f);
            
            // 1. Direct lighting
            color += computeDirectLighting(vertex);
            
            // 2. Indirect lighting (merge with light vertices)
            auto nearbyVertices = m_spatialHash.query(
                vertex.position, mergeRadius
            );
            for (auto& lightVertex : nearbyVertices) {
                color += mergeContribution(vertex, lightVertex);
            }
            
            vertex.color = vec4(color, 1.0f);
        }
    }
};
```

**Option B: Simplified Single-Bounce GI**
```cpp
// Faster approximation for real-time
vec3 computeVertexGI(const Vertex& v, const Scene& scene) {
    vec3 indirectLight = vec3(0.0f);
    
    // Sample hemisphere around vertex normal
    const int samples = 16;
    for (int i = 0; i < samples; ++i) {
        vec3 direction = sampleHemisphere(v.normal);
        
        // Trace ray to find first hit
        Hit hit = scene.raycast(v.position, direction);
        if (hit.valid) {
            // Get direct lighting at hit point
            vec3 hitLighting = computeDirectLighting(hit);
            
            // Accumulate with cosine falloff
            float NdotL = max(dot(v.normal, direction), 0.0f);
            indirectLight += hitLighting * NdotL;
        }
    }
    
    return indirectLight / float(samples);
}
```

### 8. Integration with Normal Maps

#### Vertex Colors = Lighting, Normal Maps = Detail

```glsl
// Fragment shader
void main() {
    // Sample textures
    vec3 albedo = texture(albedoMap, fragTexCoord).rgb;
    vec3 normalMap = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
    
    // Vertex color contains PRE-COMPUTED lighting
    vec3 vertexLighting = fragVertexColor.rgb;
    
    // Normal map adds surface detail WITHOUT recomputing lighting
    // Use it for micro-occlusion or detail modulation
    float normalDetail = (normalMap.r + normalMap.g + normalMap.b) / 3.0;
    vec3 detailModulation = vec3(0.9 + normalDetail * 0.2);
    
    // Combine
    vec3 finalColor = albedo * vertexLighting * detailModulation;
    
    // Optional: Add specular highlights based on normal map
    vec3 viewDir = normalize(cameraPos - fragWorldPos);
    vec3 perturbedNormal = normalize(fragTBN * normalMap);
    float spec = pow(max(dot(reflect(-viewDir, perturbedNormal), lightDir), 0.0), 32.0);
    finalColor += vec3(spec * 0.3) * vertexLighting;
    
    outColor = vec4(finalColor, 1.0);
}
```

**Key Insight:** Normal maps don't recompute lighting - they just add detail to pre-lit vertex colors!

### 9. Performance Characteristics

#### From Thesis Results
- **CPU (AMD Ryzen 7 3700X):** 28.1s for 100 iterations
- **GPU (RTX 2080, debug mode):** 105.8s for 100 iterations
- **GPU (RTX 2080, optimized):** ~2.8s estimated (10x faster)

#### Lessons Learned
1. **Memory access patterns matter more than core count**
2. **Shared memory usage critical for performance**
3. **Minimize global memory writes**
4. **Spatial coherence improves cache hits**

### 10. Recommended Approach for Our Engine

#### Phase 1: Basic Vertex Lighting
```cpp
// Start simple - direct lighting only
for (auto& vertex : mesh.vertices) {
    vec3 lighting = vec3(0.0f);
    
    // Accumulate from all lights
    for (auto& light : lights) {
        float NdotL = max(dot(vertex.normal, lightDir), 0.0f);
        float attenuation = calculateAttenuation(light, vertex.position);
        lighting += light.color * light.intensity * NdotL * attenuation;
    }
    
    // Add ambient
    lighting += ambientColor * ambientIntensity;
    
    vertex.color = vec4(lighting, 1.0f);
}
```

#### Phase 2: Add Single-Bounce GI
```cpp
// Add indirect lighting
vec3 indirectLight = sampleHemisphereGI(vertex, scene);
lighting += indirectLight * giIntensity;
```

#### Phase 3: Add Light Vertex Caching
```cpp
// Cache light vertices for reuse
updateLightVertices();  // When lights change
mergeWithLightVertices(vertex, lightVertices, mergeRadius);
```

#### Phase 4: GPU Acceleration
```cpp
// Move to compute shader
computeVertexLighting<<<gridSize, blockSize>>>(
    vertices, lights, lightVertices, frameBuffer
);
```



## Key Takeaways for Our Engine

### 1. Vertex-Based Lighting is Viable
- ✅ Store all lighting in vertex colors (4 bytes per vertex)
- ✅ No lightmap textures needed
- ✅ Fully dynamic lighting
- ✅ Works with normal maps for surface detail

### 2. Two-Pass Architecture
- **Pass 1:** Trace light paths, store light vertices
- **Pass 2:** Trace camera paths, connect/merge with light vertices
- **Result:** Direct + indirect lighting in vertex colors

### 3. Progressive Refinement
- Use adaptive merge radius: `r_i = r_1 / sqrt(i - 1)`
- Early iterations: Fast, slightly blurred
- Later iterations: Precise, noise-free

### 4. GPU Optimization Critical
- Shared memory for frequently accessed data
- Minimize global memory writes
- Spatial coherence for cache efficiency
- Parallel processing per pixel/vertex

### 5. MIS for Quality
- Combine multiple techniques (direct, BDPT, PM)
- Weight by variance to reduce noise
- Better quality than any single technique

### 6. Normal Maps Add Detail
- Vertex colors = lighting (low frequency)
- Normal maps = surface detail (high frequency)
- No need to recompute lighting in fragment shader

### 7. Practical Simplifications
- **Full VCM:** Too expensive for real-time
- **Simplified:** Pre-compute light vertices, merge per frame
- **Hybrid:** Static GI + dynamic direct lighting
- **LOD:** Reduce vertex density for distant objects

## Implementation Roadmap

### Milestone 1: Basic Vertex Lighting (1 week)
- [ ] Implement direct lighting at vertices
- [ ] Store in vertex colors (RGBA8)
- [ ] Fragment shader samples textures only
- [ ] Test with multiple dynamic lights

### Milestone 2: Hemisphere GI (1 week)
- [ ] Add simple hemisphere sampling
- [ ] Sky/ground color blending
- [ ] Height-based bounce lighting
- [ ] Ambient occlusion approximation

### Milestone 3: Light Vertex System (2 weeks)
- [ ] Implement light path tracing
- [ ] Store light vertices in spatial hash
- [ ] Merge camera vertices with light vertices
- [ ] Adaptive merge radius

### Milestone 4: GPU Acceleration (2 weeks)
- [ ] Port to Vulkan compute shader
- [ ] Optimize memory access patterns
- [ ] Use shared memory for lights/materials
- [ ] Profile and optimize bottlenecks

### Milestone 5: Normal Map Integration (1 week)
- [ ] Fragment shader detail enhancement
- [ ] Micro-occlusion from normal maps
- [ ] Specular highlights with perturbed normals
- [ ] Quality comparison tests

### Milestone 6: Temporal Refinement (1 week)
- [ ] Progressive accumulation across frames
- [ ] Temporal blending for smooth transitions
- [ ] Dirty region tracking for updates
- [ ] LOD-based update frequency

## Code Examples for Our Engine

### Example 1: Light Vertex Structure
```cpp
// core/rendering/LightVertex.h
#pragma once
#include <glm/glm.hpp>

namespace Rendering {

struct LightVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 throughput;      // Accumulated light energy
    glm::vec3 emission;        // Direct emission
    float connectionPdf;       // For MIS weight
    float mergingPdf;          // For MIS weight
    uint32_t pathLength;
    uint32_t lightIndex;       // Which light created this
    
    // Calculate contribution when merging with camera vertex
    glm::vec3 getMergeContribution(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraNormal,
        float mergeRadius
    ) const {
        float distance = glm::length(position - cameraPos);
        if (distance > mergeRadius) return glm::vec3(0.0f);
        
        // Kernel function (Gaussian-like falloff)
        float kernel = exp(-distance * distance / (mergeRadius * mergeRadius));
        
        // Geometric term
        float NdotL = glm::max(glm::dot(cameraNormal, 
            glm::normalize(position - cameraPos)), 0.0f);
        
        return throughput * kernel * NdotL;
    }
};

} // namespace Rendering
```

### Example 2: Spatial Hash for Light Vertices
```cpp
// core/rendering/LightVertexHash.h
#pragma once
#include "LightVertex.h"
#include <unordered_map>
#include <vector>

namespace Rendering {

class LightVertexHash {
public:
    void build(const std::vector<LightVertex>& vertices, float cellSize) {
        m_cellSize = cellSize;
        m_grid.clear();
        
        for (size_t i = 0; i < vertices.size(); ++i) {
            uint64_t hash = hashPosition(vertices[i].position);
            m_grid[hash].push_back(i);
        }
    }
    
    std::vector<size_t> query(const glm::vec3& position, float radius) const {
        std::vector<size_t> result;
        
        // Query neighboring cells
        int cellRadius = static_cast<int>(ceil(radius / m_cellSize));
        glm::ivec3 centerCell = worldToCell(position);
        
        for (int x = -cellRadius; x <= cellRadius; ++x) {
            for (int y = -cellRadius; y <= cellRadius; ++y) {
                for (int z = -cellRadius; z <= cellRadius; ++z) {
                    glm::ivec3 cell = centerCell + glm::ivec3(x, y, z);
                    uint64_t hash = cellToHash(cell);
                    
                    auto it = m_grid.find(hash);
                    if (it != m_grid.end()) {
                        result.insert(result.end(), 
                            it->second.begin(), it->second.end());
                    }
                }
            }
        }
        
        return result;
    }
    
private:
    float m_cellSize;
    std::unordered_map<uint64_t, std::vector<size_t>> m_grid;
    
    glm::ivec3 worldToCell(const glm::vec3& pos) const {
        return glm::ivec3(
            floor(pos.x / m_cellSize),
            floor(pos.y / m_cellSize),
            floor(pos.z / m_cellSize)
        );
    }
    
    uint64_t cellToHash(const glm::ivec3& cell) const {
        return (static_cast<uint64_t>(cell.x) << 42) |
               (static_cast<uint64_t>(cell.y) << 21) |
               static_cast<uint64_t>(cell.z);
    }
    
    uint64_t hashPosition(const glm::vec3& pos) const {
        return cellToHash(worldToCell(pos));
    }
};

} // namespace Rendering
```

### Example 3: Vertex Lighting Computer
```cpp
// core/rendering/VertexLightingComputer.h
#pragma once
#include "LightVertex.h"
#include "LightVertexHash.h"
#include <execution>

namespace Rendering {

class VertexLightingComputer {
public:
    struct Params {
        std::vector<LightVertex> lightVertices;
        LightVertexHash spatialHash;
        float mergeRadius;
        glm::vec3 ambientColor;
        float ambientIntensity;
    };
    
    void computeLighting(
        std::span<Vertex> vertices,
        const Params& params
    ) {
        // Parallel processing
        std::vector<size_t> indices(vertices.size());
        std::iota(indices.begin(), indices.end(), 0);
        
        std::for_each(std::execution::par, 
            indices.begin(), indices.end(),
            [&](size_t i) {
                computeVertexLighting(vertices[i], params);
            }
        );
    }
    
private:
    void computeVertexLighting(
        Vertex& vertex,
        const Params& params
    ) {
        glm::vec3 lighting = params.ambientColor * params.ambientIntensity;
        
        // Query nearby light vertices
        auto nearbyIndices = params.spatialHash.query(
            vertex.position, params.mergeRadius
        );
        
        // Merge with light vertices
        for (size_t idx : nearbyIndices) {
            const auto& lightVertex = params.lightVertices[idx];
            
            glm::vec3 contribution = lightVertex.getMergeContribution(
                vertex.position,
                vertex.normal,
                params.mergeRadius
            );
            
            // MIS weight (simplified)
            float weight = 1.0f / (1.0f + float(nearbyIndices.size()));
            
            lighting += contribution * weight;
        }
        
        // Store in vertex color
        vertex.color = glm::vec4(glm::clamp(lighting, 0.0f, 1.0f), 1.0f);
    }
};

} // namespace Rendering
```

### Example 4: Vulkan Compute Shader
```glsl
#version 450

layout(local_size_x = 256) in;

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
    vec4 color;  // Output
};

struct LightVertex {
    vec3 position;
    vec3 normal;
    vec3 throughput;
    float mergingPdf;
};

layout(std430, set = 0, binding = 0) buffer VertexBuffer {
    Vertex vertices[];
};

layout(std430, set = 0, binding = 1) readonly buffer LightVertexBuffer {
    LightVertex lightVertices[];
};

layout(push_constant) uniform PushConstants {
    uint vertexCount;
    uint lightVertexCount;
    float mergeRadius;
    vec3 ambientColor;
    float ambientIntensity;
} pc;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= pc.vertexCount) return;
    
    Vertex v = vertices[idx];
    vec3 lighting = pc.ambientColor * pc.ambientIntensity;
    
    // Merge with all light vertices (brute force for now)
    for (uint i = 0; i < pc.lightVertexCount; ++i) {
        LightVertex lv = lightVertices[i];
        
        float distance = length(lv.position - v.position);
        if (distance < pc.mergeRadius) {
            // Kernel
            float kernel = exp(-distance * distance / 
                (pc.mergeRadius * pc.mergeRadius));
            
            // Geometric term
            vec3 lightDir = normalize(lv.position - v.position);
            float NdotL = max(dot(v.normal, lightDir), 0.0);
            
            lighting += lv.throughput * kernel * NdotL;
        }
    }
    
    // Write back
    vertices[idx].color = vec4(clamp(lighting, 0.0, 1.0), 1.0);
}
```

## References from Thesis

1. **Kajiya 1986** - The Rendering Equation (foundation)
2. **Veach 1995** - Multiple Importance Sampling
3. **Jensen 1996** - Photon Mapping
4. **Veach 1998** - Bidirectional Path Tracing
5. **Hachisuka 2008** - Progressive Photon Mapping
6. **Georgiev 2012** - Vertex Connection and Merging

## Conclusion

The VCM thesis provides solid theoretical foundation for vertex-based GI without lightmaps. Key insights:

1. **Vertex colors can store complete lighting** (direct + indirect)
2. **Two-pass architecture** (light tracing + camera tracing)
3. **Spatial merging** with adaptive radius for progressive refinement
4. **GPU acceleration** requires careful memory optimization
5. **Normal maps add detail** without recomputing lighting

For our engine, we should implement a **simplified real-time version**:
- Pre-compute light vertices when lights change
- Merge with mesh vertices per frame (or less frequently for distant objects)
- Store results in vertex colors
- Use normal maps for surface detail in fragment shader

This approach gives us **fully dynamic lighting** with **no lightmap overhead** and **normal map detail** - exactly what we need!

---

**Next Steps:**
1. Implement basic vertex lighting system
2. Add light vertex caching
3. Implement spatial hash for fast queries
4. Port to GPU compute shader
5. Integrate with existing renderer
6. Profile and optimize

**Status:** Analysis complete, ready for implementation planning
**Priority:** High - aligns perfectly with our vertex lighting goals
**Estimated Effort:** 6-8 weeks for full implementation
