# SecretEngine - Procedural Generation Plugin Architecture

## Overview

This document defines the architecture for a procedural generation plugin inspired by three reference implementations:
- **ilmola/generator**: C++11 procedural geometry library with shapes, paths, and meshes
- **jtsiomb/meshgen**: Live-coding procedural mesh tool with hot-reload
- **kentril0/ProceduralTerrain**: OpenGL terrain generator with Perlin noise and height-based blending

## Plugin Manifest

```json
{
  "name": "ProceduralGenerator",
  "version": "1.0.0",
  "type": "game",
  "library": "ProceduralGenerator.dll",
  "dependencies": [],
  "capabilities": ["procedural_mesh", "procedural_terrain", "noise_generation"],
  "requirements": {
    "min_engine_version": "0.1.0"
  },
  "config": {
    "enable_hot_reload": true,
    "cache_generated_meshes": true,
    "max_terrain_size": 1024
  }
}
```

## Core Concepts

### 1. Generator Architecture (from ilmola/generator)

The plugin uses a lazy evaluation model where geometry is generated on-demand:

```cpp
// Generator interface - produces vertices/indices lazily
template<typename VertexType>
class IGenerator {
public:
    virtual bool Done() const = 0;
    virtual VertexType Generate() = 0;
    virtual void Next() = 0;
    virtual size_t EstimateCount() const = 0;
};
```

### 2. Primitive Types

Following ilmola/generator's taxonomy:

#### Shapes (2D)
- CircleShape
- RectangleShape
- GridShape
- ParametricShape (user-defined function)

#### Paths (3D curves)
- LinePath
- HelixPath
- ParametricPath

#### Meshes (3D geometry)
- PlaneMesh
- BoxMesh
- SphereMesh
- CylinderMesh
- TorusMesh
- TerrainMesh (Perlin noise-based)
- IcoSphereMesh
- CapsuleMesh

### 3. Modifiers (Composable Operations)

Inspired by ilmola/generator's modifier pattern:

```cpp
// Modifier wraps a generator and transforms output
template<typename PrimitiveType>
class TranslateMesh {
    PrimitiveType primitive;
    float offset[3];
public:
    auto Vertices() const { /* transform on-the-fly */ }
};

// Chainable modifiers
auto result = TranslateMesh(
    RotateMesh(
        ScaleMesh(SphereMesh{radius: 2.0}, {0.5, 1.0, 1.0}),
        angle, axis
    ),
    {10.0, 0.0, 0.0}
);
```

Available modifiers:
- TranslateMesh
- RotateMesh
- ScaleMesh
- MergeMesh (combine multiple meshes)
- SubdivideMesh
- ExtrudeMesh (extrude 2D shape along path)
- LatheMesh (revolve shape around axis)
- SpherifyMesh
- RepeatMesh (array/pattern)

## Terrain Generation (from kentril0/ProceduralTerrain)

### Height Map Generation

```cpp
struct TerrainConfig {
    uint32_t width = 256;
    uint32_t height = 256;
    float scale = 50.0f;
    
    // Fractal noise parameters
    uint32_t octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    
    // Falloff for island generation
    bool use_falloff = false;
    float falloff_a = 3.0f;
    float falloff_b = 2.2f;
    
    // Height-based regions
    std::vector<TerrainRegion> regions;
};

struct TerrainRegion {
    float height_min;
    float height_max;
    float color[4];
    char texture_path[256];
    float blend_strength = 0.1f;
};
```

### Perlin Noise Implementation

```cpp
class PerlinNoise {
public:
    PerlinNoise(uint32_t seed);
    
    // Single octave
    float Noise(float x, float y) const;
    
    // Fractal/Turbulence (multiple octaves)
    float FractalNoise(float x, float y, uint32_t octaves, 
                       float persistence, float lacunarity) const;
    
    // 3D noise for volumetric effects
    float Noise3D(float x, float y, float z) const;
};
```

### Falloff Map (Island Generation)

```cpp
class FalloffMap {
public:
    static void Apply(float* heightmap, uint32_t width, uint32_t height,
                     float a, float b) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                float nx = (float)x / width * 2.0f - 1.0f;
                float ny = (float)y / height * 2.0f - 1.0f;
                float value = std::max(std::abs(nx), std::abs(ny));
                float falloff = Evaluate(value, a, b);
                heightmap[y * width + x] *= (1.0f - falloff);
            }
        }
    }
    
private:
    static float Evaluate(float value, float a, float b) {
        return std::pow(value, a) / (std::pow(value, a) + std::pow(b - b * value, a));
    }
};
```

## Hot-Reload System (from jtsiomb/meshgen)

### Live Coding Support

```cpp
class ProceduralScript {
public:
    // User-defined generator function signature
    using GeneratorFunc = void(*)(MeshBuilder& builder, const ScriptParams& params);
    
    // Watch file for changes
    void WatchFile(const char* script_path);
    
    // Recompile and reload when modified
    bool CheckAndReload();
    
    // Execute generator
    void Generate(MeshBuilder& builder, const ScriptParams& params);
    
private:
    std::string script_path_;
    time_t last_modified_;
    void* library_handle_;
    GeneratorFunc generator_func_;
};
```

### Script Example

```cpp
// user_generator.cpp - hot-reloadable
extern "C" void Generate(MeshBuilder& builder, const ScriptParams& params) {
    float radius = params.GetFloat("radius", 1.0f);
    uint32_t segments = params.GetUInt("segments", 32);
    
    // Generate procedural geometry
    for (uint32_t i = 0; i < segments; ++i) {
        float angle = (float)i / segments * 2.0f * PI;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        builder.AddVertex({x, 0.0f, z}, {0, 1, 0}, {(float)i / segments, 0});
    }
    
    // Add indices...
}
```

## Plugin Interface

### Main Plugin Class

```cpp
class ProceduralGeneratorPlugin : public IPlugin {
public:
    const char* GetName() const override { return "ProceduralGenerator"; }
    uint32_t GetVersion() const override { return 0x010000; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    
    void* GetInterface(uint32_t id) override;
    
private:
    ICore* core_ = nullptr;
    IAllocator* allocator_ = nullptr;
    ILogger* logger_ = nullptr;
    
    // Generators
    std::vector<std::unique_ptr<IMeshGenerator>> generators_;
    
    // Hot-reload scripts
    std::vector<std::unique_ptr<ProceduralScript>> scripts_;
    
    // Noise generators
    std::unique_ptr<PerlinNoise> perlin_;
    
    // Mesh cache
    std::unordered_map<uint64_t, CachedMesh> mesh_cache_;
};
```

### Public API

```cpp
// Interface ID for GetInterface()
constexpr uint32_t IID_PROCEDURAL_GENERATOR = 0x50524F43; // 'PROC'

class IProceduralGenerator {
public:
    virtual ~IProceduralGenerator() = default;
    
    // Generate primitive meshes
    virtual MeshHandle GenerateSphere(float radius, uint32_t segments) = 0;
    virtual MeshHandle GenerateBox(float width, float height, float depth) = 0;
    virtual MeshHandle GenerateCylinder(float radius, float height, uint32_t segments) = 0;
    virtual MeshHandle GenerateTorus(float major_radius, float minor_radius, 
                                     uint32_t major_segments, uint32_t minor_segments) = 0;
    virtual MeshHandle GeneratePlane(float width, float depth, 
                                     uint32_t width_segments, uint32_t depth_segments) = 0;
    
    // Generate terrain
    virtual MeshHandle GenerateTerrain(const TerrainConfig& config) = 0;
    
    // Parametric surfaces
    virtual MeshHandle GenerateParametric(ParametricFunc func, 
                                          float u_min, float u_max, uint32_t u_steps,
                                          float v_min, float v_max, uint32_t v_steps) = 0;
    
    // Extrusion and lathing
    virtual MeshHandle ExtrudeShape(const Shape2D& shape, const Path3D& path) = 0;
    virtual MeshHandle LatheShape(const Shape2D& shape, uint32_t segments, float angle) = 0;
    
    // Modifiers
    virtual MeshHandle Transform(MeshHandle mesh, const float matrix[16]) = 0;
    virtual MeshHandle Merge(const MeshHandle* meshes, uint32_t count) = 0;
    virtual MeshHandle Subdivide(MeshHandle mesh, uint32_t iterations) = 0;
    
    // Noise functions
    virtual float PerlinNoise2D(float x, float y, uint32_t seed) = 0;
    virtual float FractalNoise2D(float x, float y, uint32_t octaves, 
                                 float persistence, float lacunarity, uint32_t seed) = 0;
    
    // Hot-reload scripts
    virtual bool LoadScript(const char* path, const char* name) = 0;
    virtual MeshHandle ExecuteScript(const char* name, const ScriptParams& params) = 0;
    
    // Mesh export
    virtual bool ExportMesh(MeshHandle mesh, const char* path, MeshFormat format) = 0;
};
```

## Mesh Builder Utility

```cpp
class MeshBuilder {
public:
    struct Vertex {
        float position[3];
        float normal[3];
        float texcoord[2];
        float color[4] = {1, 1, 1, 1};
    };
    
    MeshBuilder(IAllocator* allocator);
    
    // Add geometry
    uint32_t AddVertex(const float pos[3], const float normal[3], const float uv[2]);
    void AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2);
    void AddQuad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3);
    
    // Utilities
    void CalculateNormals();
    void CalculateTangents();
    void Optimize(); // Remove duplicates, reorder for cache
    
    // Export to engine format
    bool WriteMeshBin(const char* path);
    
    // Access
    const std::vector<Vertex>& GetVertices() const { return vertices_; }
    const std::vector<uint32_t>& GetIndices() const { return indices_; }
    
private:
    IAllocator* allocator_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
};
```

## Integration with SecretEngine

### Component Integration

```cpp
struct ProceduralMeshComponent {
    char generator_type[64] = "sphere"; // "sphere", "box", "terrain", "script"
    char script_name[256] = {0};        // For script-based generation
    
    // Parameters (JSON-serializable)
    char params_json[1024] = "{}";
    
    // Generation flags
    bool regenerate_on_load = true;
    bool cache_result = true;
    uint64_t cache_key = 0;
    
    // Output
    char generated_mesh_path[256] = {0};
    
    static const uint32_t TypeID = 0x0010;
};
```

### System Integration

```cpp
// In game logic or level loading
void GenerateProceduralMeshes(IWorld* world, IProceduralGenerator* procgen) {
    auto entities = world->GetEntitiesWithComponent(ProceduralMeshComponent::TypeID);
    
    for (EntityID entity : entities) {
        auto* proc_comp = world->GetComponent<ProceduralMeshComponent>(entity);
        auto* mesh_comp = world->GetComponent<MeshComponent>(entity);
        
        if (!proc_comp || !mesh_comp) continue;
        
        // Parse parameters
        ScriptParams params;
        params.ParseJSON(proc_comp->params_json);
        
        // Generate mesh
        MeshHandle handle;
        if (strcmp(proc_comp->generator_type, "sphere") == 0) {
            float radius = params.GetFloat("radius", 1.0f);
            uint32_t segments = params.GetUInt("segments", 32);
            handle = procgen->GenerateSphere(radius, segments);
        }
        else if (strcmp(proc_comp->generator_type, "terrain") == 0) {
            TerrainConfig config;
            // ... parse config from params
            handle = procgen->GenerateTerrain(config);
        }
        else if (strcmp(proc_comp->generator_type, "script") == 0) {
            handle = procgen->ExecuteScript(proc_comp->script_name, params);
        }
        
        // Export and assign
        char output_path[512];
        snprintf(output_path, sizeof(output_path), 
                "Assets/meshes/generated/%s_%llu.meshbin", 
                proc_comp->generator_type, entity);
        
        procgen->ExportMesh(handle, output_path, MeshFormat::MeshBin);
        strcpy(mesh_comp->meshPath, output_path);
    }
}
```

## JSON Level Integration

```json
{
  "entities": [
    {
      "name": "ProceduralTerrain",
      "components": {
        "Transform": {
          "position": [0, 0, 0],
          "rotation": [0, 0, 0],
          "scale": [1, 1, 1]
        },
        "ProceduralMesh": {
          "generator_type": "terrain",
          "params_json": "{\"width\":512,\"height\":512,\"scale\":100.0,\"octaves\":6,\"persistence\":0.5,\"use_falloff\":true}",
          "regenerate_on_load": false,
          "cache_result": true
        },
        "Mesh": {
          "meshPath": "meshes/generated/terrain_12345.meshbin",
          "color": [1, 1, 1, 1]
        }
      }
    },
    {
      "name": "CustomProceduralMesh",
      "components": {
        "Transform": {
          "position": [10, 0, 0]
        },
        "ProceduralMesh": {
          "generator_type": "script",
          "script_name": "my_custom_generator",
          "params_json": "{\"complexity\":5,\"seed\":42}",
          "regenerate_on_load": true
        },
        "Mesh": {
          "meshPath": "meshes/generated/custom_67890.meshbin"
        }
      }
    }
  ]
}
```

## Performance Considerations

### 1. Lazy Evaluation
- Generators produce vertices on-demand
- No intermediate storage for large meshes
- Stream directly to GPU buffers when possible

### 2. Caching
- Hash generator parameters to create cache keys
- Store generated meshes to disk
- LRU cache for frequently used procedural meshes

### 3. Multithreading
- Generate meshes on job system threads
- Partition terrain generation into chunks
- Parallel noise evaluation

### 4. LOD Support
```cpp
struct LODConfig {
    uint32_t lod_levels = 4;
    float lod_distances[8] = {10, 25, 50, 100};
    float lod_scale_factors[8] = {1.0, 0.5, 0.25, 0.125};
};

MeshHandle GenerateTerrainLOD(const TerrainConfig& config, 
                              const LODConfig& lod_config);
```

## Example Usage Scenarios

### Scenario 1: Runtime Terrain Generation
```cpp
// Generate island terrain at runtime
TerrainConfig config;
config.width = 512;
config.height = 512;
config.scale = 100.0f;
config.octaves = 6;
config.use_falloff = true;

// Add height-based regions
config.regions.push_back({0.0f, 0.3f, {0.1, 0.3, 0.8, 1}, "textures/water.jpg"});
config.regions.push_back({0.3f, 0.5f, {0.8, 0.7, 0.4, 1}, "textures/sand.jpg"});
config.regions.push_back({0.5f, 0.8f, {0.2, 0.6, 0.2, 1}, "textures/grass.jpg"});
config.regions.push_back({0.8f, 1.0f, {0.5, 0.5, 0.5, 1}, "textures/rock.jpg"});

MeshHandle terrain = procgen->GenerateTerrain(config);
procgen->ExportMesh(terrain, "Assets/meshes/island.meshbin", MeshFormat::MeshBin);
```

### Scenario 2: Procedural Building Generation
```cpp
// Hot-reloadable building generator script
// buildings.cpp
extern "C" void Generate(MeshBuilder& builder, const ScriptParams& params) {
    uint32_t floors = params.GetUInt("floors", 5);
    float floor_height = params.GetFloat("floor_height", 3.0f);
    float width = params.GetFloat("width", 10.0f);
    
    for (uint32_t floor = 0; floor < floors; ++floor) {
        float y = floor * floor_height;
        // Generate floor geometry...
        GenerateFloor(builder, y, width);
        GenerateWindows(builder, y, width, floor);
    }
    GenerateRoof(builder, floors * floor_height, width);
}
```

### Scenario 3: Modifier Chains
```cpp
// Create complex geometry through composition
auto sphere = SphereMesh{1.0f, 32};
auto scaled = ScaleMesh(sphere, {1.0f, 2.0f, 1.0f}); // Ellipsoid
auto rotated = RotateMesh(scaled, PI/4, {0, 1, 0});
auto translated = TranslateMesh(rotated, {5, 0, 0});

MeshHandle handle = procgen->GenerateFromPrimitive(translated);
```

## Future Extensions

1. **Volumetric Generation**: Marching cubes for caves, overhangs
2. **Vegetation Placement**: Procedural tree/grass distribution based on terrain
3. **Erosion Simulation**: Hydraulic/thermal erosion for realistic terrain
4. **Road/Path Generation**: Spline-based path networks
5. **City Generation**: Building placement, road networks
6. **Dungeon Generation**: BSP/cellular automata for interiors
7. **GPU Acceleration**: Compute shaders for noise and mesh generation

## References

- [ilmola/generator](https://github.com/ilmola/generator) - Lazy evaluation, modifier pattern
- [jtsiomb/meshgen](https://github.com/jtsiomb/meshgen) - Hot-reload system
- [kentril0/ProceduralTerrain](https://github.com/kentril0/ProceduralTerrain) - Perlin noise, height-based blending

## Status

📋 DESIGN PHASE
Ready for implementation as a SecretEngine plugin.
