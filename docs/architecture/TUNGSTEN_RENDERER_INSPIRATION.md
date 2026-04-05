# Tungsten Renderer Inspiration for SecretEngine

## Overview

This document outlines features and techniques from the [Tungsten Renderer](https://github.com/tunabrain/tungsten) by tunabrain that can inspire improvements to SecretEngine's rendering architecture.

**Tungsten Highlights:**
- Physically based renderer written in C++11
- Unbiased light transport simulation
- Multiple light transport algorithms (bidirectional path tracing, photon mapping, metropolis light transport)
- Intel Embree integration for high-performance ray intersection
- Full multicore utilization
- Modular architecture with clear separation of concerns

---

## Key Features to Adopt

### 1. Architecture & Code Structure

**Tungsten's Organization:**
```
src/core/          - Primitive intersection, materials, sampling, integration
src/thirdparty/    - Third-party libraries
src/tungsten/      - Command line interface to core
src/utilities/     - Additional tools (server, scene manipulation, HDR tools)
```

**Apply to SecretEngine:**
```
core/              - Core engine interfaces and systems
plugins/           - Modular rendering, physics, gameplay systems
tools/             - Build tools, asset converters, profilers
utilities/         - Scene manipulation, texture tools, mesh converters
```

**Benefits:**
- Clear separation between core rendering and application
- Easy to test and benchmark individual components
- Modular design allows swapping implementations

---

### 2. Material System

**Tungsten's Approach:**
- Physically based materials with proper BRDF models
- Material test scenes for validation
- JSON-based material definitions
- Support for complex material properties

**Implementation for SecretEngine:**

```cpp
// Enhanced material system inspired by Tungsten
namespace SecretEngine {

struct PhysicallyBasedMaterial {
    // Base color (albedo)
    float baseColor[3];
    
    // PBR parameters
    float metallic;        // 0 = dielectric, 1 = metal
    float roughness;       // 0 = smooth, 1 = rough
    float specular;        // Specular intensity (0-1)
    float specularTint;    // Tint specular with base color
    
    // Advanced properties
    float anisotropic;     // Anisotropic specular (0-1)
    float sheen;           // Sheen for cloth-like materials
    float sheenTint;       // Sheen color tint
    float clearcoat;       // Clear coat layer (0-1)
    float clearcoatGloss;  // Clear coat glossiness
    
    // Subsurface scattering
    float subsurface;      // SSS amount (0-1)
    float subsurfaceColor[3];
    
    // Transmission
    float transmission;    // Glass-like transmission (0-1)
    float ior;            // Index of refraction (1.0-2.5)
    
    // Emission
    float emission[3];
    float emissionStrength;
    
    // Texture indices
    uint32_t baseColorTexture;
    uint32_t normalTexture;
    uint32_t metallicRoughnessTexture;
    uint32_t emissionTexture;
    uint32_t occlusionTexture;
};

class IMaterialSystem {
public:
    virtual ~IMaterialSystem() = default;
    
    // Material management
    virtual uint32_t CreateMaterial(const PhysicallyBasedMaterial& material) = 0;
    virtual void UpdateMaterial(uint32_t materialID, const PhysicallyBasedMaterial& material) = 0;
    virtual void RemoveMaterial(uint32_t materialID) = 0;
    
    // Material test scene (like Tungsten's material test ball)
    virtual void CreateMaterialTestScene(uint32_t materialID) = 0;
    
    // BRDF evaluation
    virtual void EvaluateBRDF(const PhysicallyBasedMaterial& material,
                             const float* viewDir,
                             const float* lightDir,
                             const float* normal,
                             float* outColor) = 0;
};

} // namespace SecretEngine
```

---

### 3. Scene Management

**Tungsten's Scene Format:**
- JSON-based scene descriptions
- Hierarchical object organization
- Material assignments
- Camera setup
- Light definitions

**Example Scene Structure:**
```json
{
    "renderer": {
        "type": "path_tracer",
        "max_bounces": 8,
        "spp": 256
    },
    "camera": {
        "type": "pinhole",
        "transform": {
            "position": [0, 2, 5],
            "look_at": [0, 0, 0],
            "up": [0, 1, 0]
        },
        "fov": 60
    },
    "primitives": [
        {
            "type": "mesh",
            "file": "models/scene.wo3",
            "material": "default_material"
        }
    ],
    "materials": {
        "default_material": {
            "type": "lambert",
            "albedo": [0.8, 0.8, 0.8]
        }
    }
}
```

**Apply to SecretEngine:**
- Extend existing JSON scene format
- Add material library support
- Support hierarchical transforms
- Enable scene composition

---

### 4. High-Performance Geometry Intersection

**Tungsten uses Intel Embree:**
- Hardware-accelerated ray tracing
- BVH (Bounding Volume Hierarchy) construction
- SIMD optimizations
- Multi-threaded traversal

**For SecretEngine (Vulkan-based):**

```cpp
// Use Vulkan Ray Tracing extensions
namespace SecretEngine {

class RayTracingAccelerationStructure {
public:
    // Build acceleration structure for static geometry
    void BuildBLAS(const std::vector<MeshData>& meshes);
    
    // Build top-level acceleration structure
    void BuildTLAS(const std::vector<InstanceData>& instances);
    
    // Update for dynamic objects
    void UpdateTLAS(const std::vector<InstanceData>& instances);
    
    // Ray tracing queries
    struct RayHit {
        float distance;
        uint32_t primitiveID;
        uint32_t instanceID;
        float barycentrics[2];
    };
    
    bool TraceRay(const float* origin, const float* direction, 
                  float tMin, float tMax, RayHit& hit);
};

} // namespace SecretEngine
```

**Benefits:**
- Hardware-accelerated ray tracing on modern GPUs
- Efficient BVH for complex scenes
- Support for hybrid rasterization + ray tracing

---

### 5. Multicore Utilization

**Tungsten's Approach:**
- Tile-based rendering for parallelization
- Work stealing for load balancing
- Lock-free data structures where possible

**Apply to SecretEngine:**

```cpp
// Parallel rendering inspired by Tungsten
namespace SecretEngine {

class ParallelRenderer {
public:
    struct RenderTile {
        uint32_t x, y;
        uint32_t width, height;
        uint32_t sampleCount;
    };
    
    // Divide framebuffer into tiles
    std::vector<RenderTile> GenerateTiles(uint32_t fbWidth, uint32_t fbHeight,
                                          uint32_t tileSize = 64);
    
    // Render tiles in parallel using job system
    void RenderTilesParallel(const std::vector<RenderTile>& tiles,
                            const Scene& scene,
                            Framebuffer& output);
    
    // Progressive rendering (like Tungsten's progressive photon mapping)
    void RenderProgressive(const Scene& scene,
                          Framebuffer& output,
                          uint32_t maxSamples);
};

} // namespace SecretEngine
```

---

### 6. HDR Pipeline & Tonemapping

**Tungsten's HDR Tools:**
- HDR framebuffer output (.exr format)
- Multiple tonemapping operators (filmic, Reinhard, etc.)
- Exposure adjustment
- HDR image merging for distributed rendering

**Implementation:**

```cpp
namespace SecretEngine {

enum class TonemapOperator {
    Reinhard,
    Filmic,
    ACES,
    Uncharted2,
    None
};

class HDRPipeline {
public:
    // Render to HDR framebuffer
    void RenderToHDR(VkCommandBuffer cmd, const Scene& scene);
    
    // Apply tonemapping
    void Tonemap(VkCommandBuffer cmd,
                VkImage hdrImage,
                VkImage ldrOutput,
                TonemapOperator op,
                float exposure = 0.0f);
    
    // Exposure adjustment
    void AdjustExposure(VkImage hdrImage, float exposureEV);
    
    // Merge multiple HDR images (for distributed rendering)
    void MergeHDRImages(const std::vector<VkImage>& inputs,
                       VkImage output);
    
    // Auto-exposure calculation
    float CalculateAutoExposure(VkImage hdrImage);
};

} // namespace SecretEngine
```

**Tonemapping Operators:**

```glsl
// Reinhard tonemap
vec3 ReinhardTonemap(vec3 hdrColor) {
    return hdrColor / (hdrColor + vec3(1.0));
}

// Filmic tonemap (Uncharted 2)
vec3 Uncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// ACES filmic
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}
```

---

### 7. Render Server Architecture

**Tungsten's tungsten_server:**
- Built-in HTTP status server
- Real-time framebuffer access
- JSON status information
- Render log streaming

**Apply to SecretEngine:**

```cpp
namespace SecretEngine {

class RenderServer {
public:
    // Start HTTP server on specified port
    void Start(uint16_t port = 8080);
    
    // Stop server
    void Stop();
    
    // Endpoints:
    // GET /render - Current framebuffer (PNG)
    // GET /status - JSON render status
    // GET /log - Text render log
    // POST /control - Control commands (pause, resume, stop)
    
    struct RenderStatus {
        uint32_t currentFrame;
        uint32_t totalFrames;
        float progress;
        float fps;
        uint32_t samplesPerPixel;
        std::string currentPass;
        float elapsedTime;
    };
    
    RenderStatus GetStatus() const;
};

} // namespace SecretEngine
```

**Use Cases:**
- Remote monitoring of renders
- Distributed rendering coordination
- Real-time preview in web browser
- Integration with render farms

---

### 8. Asset Pipeline

**Tungsten's Tools:**
- `obj2json` - Convert OBJ to native format
- `json2xml` - Export to other renderers
- Native `.wo3` mesh format for fast loading
- Automatic mesh conversion

**For SecretEngine:**

```cpp
namespace SecretEngine::Tools {

class AssetConverter {
public:
    // Convert OBJ to native .meshbin format
    bool ConvertOBJ(const std::string& objPath,
                   const std::string& meshbinPath);
    
    // Convert GLTF to native format
    bool ConvertGLTF(const std::string& gltfPath,
                    const std::string& outputPath);
    
    // Batch conversion
    void ConvertDirectory(const std::string& inputDir,
                         const std::string& outputDir,
                         const std::string& format);
    
    // Scene packaging (like Tungsten's scenemanip)
    bool PackageScene(const std::string& sceneJson,
                     const std::string& outputZip);
};

} // namespace SecretEngine::Tools
```

---

### 9. Material Test Scene

**Tungsten's Material Test Ball:**
- Standard test geometry
- Controlled lighting
- Easy material validation

**Create for SecretEngine:**

```cpp
namespace SecretEngine {

class MaterialTestScene {
public:
    // Create standard test scene with sphere
    void CreateTestBall(uint32_t materialID);
    
    // Create test scene with multiple primitives
    void CreateTestSuite(uint32_t materialID);
    
    // Standard lighting setups
    enum class LightingPreset {
        Studio,      // 3-point lighting
        Outdoor,     // Sun + sky
        Indoor,      // Area lights
        Neutral      // Uniform lighting
    };
    
    void SetLighting(LightingPreset preset);
    
    // Render material preview
    void RenderPreview(uint32_t materialID,
                      uint32_t width, uint32_t height,
                      const std::string& outputPath);
};

} // namespace SecretEngine
```

---

## Implementation Roadmap

### Phase 1: Material System (Week 1-2)
- [ ] Implement PhysicallyBasedMaterial structure
- [ ] Add IMaterialSystem interface
- [ ] Create material test scene
- [ ] Implement basic BRDF evaluation

### Phase 2: HDR Pipeline (Week 3-4)
- [ ] HDR framebuffer support
- [ ] Implement tonemapping operators
- [ ] Add exposure control
- [ ] Auto-exposure calculation

### Phase 3: Scene Management (Week 5-6)
- [ ] Enhanced JSON scene format
- [ ] Material library system
- [ ] Scene composition tools
- [ ] Hierarchical transforms

### Phase 4: Performance (Week 7-8)
- [ ] Tile-based parallel rendering
- [ ] Vulkan ray tracing integration
- [ ] BVH acceleration structures
- [ ] Multi-threaded rendering

### Phase 5: Tools & Utilities (Week 9-10)
- [ ] Asset converter tools
- [ ] Render server (HTTP API)
- [ ] Scene packaging utility
- [ ] Material preview generator

---

## Key Takeaways

1. **Modular Architecture**: Separate core rendering from application logic
2. **Physically Based**: Use proper PBR materials and lighting models
3. **Performance First**: Multicore utilization and efficient data structures
4. **Tool Support**: Build utilities for artists and developers
5. **HDR Pipeline**: Proper HDR workflow with tonemapping
6. **Testing**: Material test scenes for validation
7. **Extensibility**: JSON-based configuration and scene description

---

## References

1. [Tungsten Renderer](https://github.com/tunabrain/tungsten)
   - tunabrain
   
2. [Intel Embree](https://www.embree.org/)
   - High-performance ray tracing kernels

3. [Physically Based Rendering Book](https://www.pbr-book.org/)
   - Matt Pharr, Wenzel Jakob, Greg Humphreys

4. [Real-Time Rendering](https://www.realtimerendering.com/)
   - Tomas Akenine-Möller, Eric Haines, Naty Hoffman

---

*Document created: 2026-04-05*
*Based on Tungsten Renderer by tunabrain*
