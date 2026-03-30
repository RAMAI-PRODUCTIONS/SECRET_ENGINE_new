# 🎨 Material & Shader System - Renderer-Independent Design

## 🎯 GOAL

Create a material and shader system that is:
- **Renderer-independent** - Works with Vulkan, DX12, Metal, etc.
- **High-performance** - GPU-driven, zero CPU overhead
- **Flexible** - Easy to create material variations
- **Hot-reloadable** - Shaders reload without restart (development)

---

## 🏗️ ARCHITECTURE

```
┌─────────────────────────────────────────────────────────────────┐
│                      MATERIAL SYSTEM                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│                    ┌──────────────────┐                         │
│                    │   Core Engine    │                         │
│                    │   (Interfaces)   │                         │
│                    └────────┬─────────┘                         │
│                             │                                    │
│              ┌──────────────┼──────────────┐                    │
│              │              │              │                    │
│     ┌────────▼────────┐ ┌──▼──────────┐ ┌─▼─────────────┐     │
│     │ IMaterialSystem │ │IShaderSystem │ │ITextureSystem │     │
│     └────────┬────────┘ └──┬──────────┘ └─┬─────────────┘     │
│              │              │              │                    │
│     ┌────────▼────────┐ ┌──▼──────────┐ ┌─▼─────────────┐     │
│     │ MaterialSystem  │ │ShaderCompiler│ │TextureSystem  │     │
│     │    Plugin       │ │   (SPIRV)    │ │   Plugin      │     │
│     │                 │ │              │ │               │     │
│     │ • PBR materials │ │ • GLSL→SPIRV │ │ • PNG/ASTC    │     │
│     │ • Material DB   │ │ • Hot reload │ │ • Streaming   │     │
│     │ • Variations    │ │ • Caching    │ │ • Compression │     │
│     └────────┬────────┘ └──┬──────────┘ └─┬─────────────┘     │
│              │              │              │                    │
│              └──────────────┼──────────────┘                    │
│                             │                                    │
│                    ┌────────▼─────────┐                         │
│                    │ VulkanRenderer   │                         │
│                    │  (Backend)       │                         │
│                    │                  │                         │
│                    │ • Material SSBO  │                         │
│                    │ • Shader modules │                         │
│                    │ • Descriptor sets│                         │
│                    └──────────────────┘                         │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📦 COMPONENT BREAKDOWN

### 1. IMaterialSystem Interface

**Location**: `core/include/SecretEngine/IMaterialSystem.h`

```cpp
namespace SecretEngine::Materials {

// Material properties (PBR workflow)
struct MaterialProperties {
    // Base properties
    float baseColor[4];          // RGBA
    float metallic;              // 0.0 = dielectric, 1.0 = metal
    float roughness;             // 0.0 = smooth, 1.0 = rough
    float emissive[3];           // RGB emissive color
    float emissiveStrength;      // Emissive multiplier
    
    // Texture indices (bindless)
    uint32_t albedoTexture;
    uint32_t normalTexture;
    uint32_t metallicRoughnessTexture;
    uint32_t emissiveTexture;
    uint32_t aoTexture;
    
    // Flags
    uint32_t flags;              // Transparent, DoubleSided, etc.
    
    // Padding for GPU alignment
    float padding[2];
};

// Material handle
struct MaterialHandle {
    uint32_t id;
    uint32_t generation;  // For handle validation
};

class IMaterialSystem {
public:
    virtual ~IMaterialSystem() = default;
    
    // Material management
    virtual MaterialHandle CreateMaterial(const char* name, const MaterialProperties& props) = 0;
    virtual void UpdateMaterial(MaterialHandle handle, const MaterialProperties& props) = 0;
    virtual void DestroyMaterial(MaterialHandle handle) = 0;
    virtual const MaterialProperties* GetMaterial(MaterialHandle handle) const = 0;
    virtual MaterialHandle GetMaterialByName(const char* name) const = 0;
    
    // Material instances (variations)
    virtual MaterialHandle CreateMaterialInstance(MaterialHandle parent, const char* name) = 0;
    virtual void SetMaterialParameter(MaterialHandle handle, const char* param, const void* data, size_t size) = 0;
    
    // Batch operations
    virtual void BeginMaterialUpdate() = 0;
    virtual void EndMaterialUpdate() = 0;
    
    // GPU data access (for renderer backend)
    virtual const void* GetMaterialBuffer() const = 0;
    virtual size_t GetMaterialBufferSize() const = 0;
    virtual uint32_t GetMaterialCount() const = 0;
};

} // namespace SecretEngine::Materials
```

### 2. IShaderSystem Interface

**Location**: `core/include/SecretEngine/IShaderSystem.h`

```cpp
namespace SecretEngine::Shaders {

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessControl,
    TessEval
};

struct ShaderHandle {
    uint32_t id;
    void* nativeHandle;  // Renderer-specific (VkShaderModule, etc.)
};

struct ShaderCompileOptions {
    ShaderStage stage;
    const char* entryPoint;
    const char* defines[16];  // Preprocessor defines
    int defineCount;
    bool optimize;
    bool debug;
};

class IShaderSystem {
public:
    virtual ~IShaderSystem() = default;
    
    // Shader compilation
    virtual ShaderHandle CompileFromSource(const char* source, const ShaderCompileOptions& options) = 0;
    virtual ShaderHandle CompileFromFile(const char* path, const ShaderCompileOptions& options) = 0;
    virtual ShaderHandle CompileFromSPIRV(const uint32_t* spirv, size_t size, ShaderStage stage) = 0;
    
    // Shader management
    virtual void DestroyShader(ShaderHandle handle) = 0;
    virtual bool IsValid(ShaderHandle handle) const = 0;
    
    // Hot reload (development)
    virtual void EnableHotReload(bool enable) = 0;
    virtual void ReloadShader(ShaderHandle handle) = 0;
    virtual void ReloadAllShaders() = 0;
    
    // Shader reflection
    virtual void GetShaderInputs(ShaderHandle handle, char names[][64], int& count) const = 0;
    virtual void GetShaderOutputs(ShaderHandle handle, char names[][64], int& count) const = 0;
};

} // namespace SecretEngine::Shaders
```

---

## 🚀 IMPLEMENTATION PLAN

### Phase 1: Core Interfaces (30 minutes)

**Create**:
- `core/include/SecretEngine/IMaterialSystem.h`
- `core/include/SecretEngine/IShaderSystem.h`

**Define**:
- Material properties structure
- Shader compilation interface
- Handle-based API

### Phase 2: MaterialSystem Plugin (1.5 hours)

**Create**:
```
plugins/MaterialSystem/
├── CMakeLists.txt
├── src/
│   ├── MaterialPlugin.h
│   ├── MaterialPlugin.cpp
│   ├── MaterialManager.h
│   ├── MaterialManager.cpp
│   ├── MaterialDatabase.h         // Material storage
│   ├── MaterialDatabase.cpp
│   └── MaterialTypes.h
```

**Implement**:
- Material creation/destruction
- Material database (hash map)
- Material instances (variations)
- GPU buffer management (SSBO)

**Key Features**:
- Handle-based API (safe, versioned)
- Material instances for variations
- Batch updates for performance
- GPU-friendly data layout

### Phase 3: ShaderCompiler (1 hour)

**Create**:
```
plugins/MaterialSystem/src/
├── ShaderCompiler.h
├── ShaderCompiler.cpp
├── SPIRVReflection.h              // Shader reflection
└── SPIRVReflection.cpp
```

**Dependencies**:
- glslang (GLSL → SPIRV)
- SPIRV-Cross (SPIRV → reflection)
- SPIRV-Tools (optimization)

**Implement**:
- GLSL to SPIRV compilation
- Shader caching (disk cache)
- Hot reload (file watching)
- Shader reflection

### Phase 4: Vulkan Backend (1.5 hours)

**Create**:
```
plugins/VulkanRenderer/src/
├── VulkanMaterialBackend.h
├── VulkanMaterialBackend.cpp
├── VulkanShaderCache.h
└── VulkanShaderCache.cpp
```

**Implement**:
- Material SSBO upload to GPU
- Shader module creation
- Descriptor set management
- Pipeline cache

**Integration**:
- Query IMaterialSystem for material data
- Upload to GPU buffers
- Bind materials in draw calls

---

## 🎨 MATERIAL WORKFLOW

### Creating a Material

```cpp
// In game code
auto* materialSystem = core->GetCapability<IMaterialSystem>("materials");

MaterialProperties props = {};
props.baseColor[0] = 1.0f;  // Red
props.baseColor[1] = 0.0f;
props.baseColor[2] = 0.0f;
props.baseColor[3] = 1.0f;
props.metallic = 0.0f;
props.roughness = 0.5f;
props.albedoTexture = textureID;

MaterialHandle material = materialSystem->CreateMaterial("RedMetal", props);
```

### Creating Material Instances

```cpp
// Create variation of base material
MaterialHandle instance = materialSystem->CreateMaterialInstance(material, "RedMetalShiny");

// Override roughness
float roughness = 0.1f;
materialSystem->SetMaterialParameter(instance, "roughness", &roughness, sizeof(float));
```

### Using Materials in Renderer

```cpp
// In VulkanRenderer
auto* materialSystem = core->GetCapability<IMaterialSystem>("materials");

// Get GPU buffer
const void* materialBuffer = materialSystem->GetMaterialBuffer();
size_t bufferSize = materialSystem->GetMaterialBufferSize();

// Upload to GPU
vkCmdUpdateBuffer(cmd, m_materialSSBO, 0, bufferSize, materialBuffer);

// Bind in shader
layout(set = 1, binding = 0) readonly buffer Materials {
    MaterialProperties materials[];
};

// Access in shader
MaterialProperties mat = materials[materialID];
vec4 color = mat.baseColor;
```

---

## 🔥 SHADER WORKFLOW

### Compiling Shaders

```cpp
// In MaterialSystem plugin
auto* shaderSystem = core->GetCapability<IShaderSystem>("shaders");

ShaderCompileOptions options = {};
options.stage = ShaderStage::Fragment;
options.entryPoint = "main";
options.optimize = true;

ShaderHandle shader = shaderSystem->CompileFromFile("shaders/pbr.frag", options);
```

### Hot Reload (Development)

```cpp
// Enable hot reload
shaderSystem->EnableHotReload(true);

// Shaders automatically reload when files change
// Or manually reload:
shaderSystem->ReloadShader(shader);
```

### Shader Variants (Preprocessor)

```cpp
ShaderCompileOptions options = {};
options.stage = ShaderStage::Fragment;
options.entryPoint = "main";
options.defines[0] = "USE_NORMAL_MAP";
options.defines[1] = "USE_EMISSIVE";
options.defineCount = 2;

ShaderHandle shader = shaderSystem->CompileFromFile("shaders/pbr.frag", options);
```

---

## ⚡ PERFORMANCE OPTIMIZATIONS

### 1. GPU-Driven Materials

**Problem**: CPU overhead from material switching

**Solution**: Store all materials in GPU buffer (SSBO)
```glsl
// Vertex shader
layout(set = 1, binding = 0) readonly buffer Materials {
    MaterialProperties materials[];
};

// Instance data includes material ID
layout(location = 0) in uint materialID;

void main() {
    MaterialProperties mat = materials[materialID];
    // Use material properties
}
```

**Benefit**: Zero CPU overhead, all materials available on GPU

### 2. Bindless Textures

**Problem**: Descriptor set switching overhead

**Solution**: Use bindless texture array
```glsl
layout(set = 2, binding = 0) uniform sampler2D textures[];

void main() {
    MaterialProperties mat = materials[materialID];
    vec4 albedo = texture(textures[mat.albedoTexture], uv);
}
```

**Benefit**: No descriptor set switching, all textures accessible

### 3. Shader Caching

**Problem**: Shader compilation is slow

**Solution**: Cache compiled SPIRV to disk
```cpp
// Check cache first
std::string cacheKey = Hash(source + options);
if (cache.Has(cacheKey)) {
    return cache.Load(cacheKey);
}

// Compile and cache
ShaderHandle shader = Compile(source, options);
cache.Save(cacheKey, shader);
```

**Benefit**: Fast startup, compile once

### 4. Material Instancing

**Problem**: Many similar materials waste memory

**Solution**: Material instances share base data
```cpp
// Base material (1 copy)
MaterialHandle base = CreateMaterial("Metal", baseProps);

// Instances (only store overrides)
MaterialHandle red = CreateMaterialInstance(base, "RedMetal");
SetMaterialParameter(red, "baseColor", redColor);

MaterialHandle blue = CreateMaterialInstance(base, "BlueMetal");
SetMaterialParameter(blue, "baseColor", blueColor);
```

**Benefit**: Memory efficient, fast variations

---

## 📊 MEMORY LAYOUT

### Material SSBO (GPU)

```cpp
// Aligned for GPU (std140)
struct MaterialProperties {
    vec4 baseColor;              // 16 bytes
    float metallic;              // 4 bytes
    float roughness;             // 4 bytes
    float emissiveStrength;      // 4 bytes
    float padding1;              // 4 bytes (alignment)
    vec3 emissive;               // 12 bytes
    uint flags;                  // 4 bytes
    uint albedoTexture;          // 4 bytes
    uint normalTexture;          // 4 bytes
    uint metallicRoughnessTexture; // 4 bytes
    uint emissiveTexture;        // 4 bytes
    uint aoTexture;              // 4 bytes
    uint padding2;               // 4 bytes (alignment)
    uint padding3;               // 4 bytes (alignment)
};  // Total: 80 bytes (aligned to 16)

// GPU buffer
layout(set = 1, binding = 0) readonly buffer Materials {
    MaterialProperties materials[MAX_MATERIALS];  // 80 * 4096 = 320KB
};
```

**Benefits**:
- Contiguous memory (cache-friendly)
- All materials on GPU (no uploads)
- Fast random access

---

## 🧪 TESTING PLAN

### Unit Tests:
- [ ] Material creation/destruction
- [ ] Material instances
- [ ] Shader compilation
- [ ] Shader caching
- [ ] Hot reload

### Integration Tests:
- [ ] VulkanRenderer uses materials
- [ ] Material switching works
- [ ] Textures bind correctly
- [ ] Shader variants work

### Performance Tests:
- [ ] Material switching overhead (should be ~0)
- [ ] Shader compilation time (< 100ms)
- [ ] Memory usage (< 1MB for 1000 materials)
- [ ] Hot reload time (< 50ms)

---

## 📈 SUCCESS METRICS

### Performance:
- **Material switching**: 0 CPU overhead (GPU-driven)
- **Shader compilation**: < 100ms per shader
- **Memory**: < 1KB per material
- **Hot reload**: < 50ms

### Quality:
- **Modularity**: Materials work with any renderer
- **Flexibility**: Easy to create variations
- **Debuggability**: Hot reload for iteration
- **Maintainability**: Clear separation of concerns

---

## 🗓️ TIMELINE

### Task 2.2: Material & Shader System (3 hours)

**Hour 1**: Core interfaces + MaterialSystem plugin
- Create IMaterialSystem.h
- Create IShaderSystem.h
- Implement MaterialManager
- Implement MaterialDatabase

**Hour 2**: ShaderCompiler + Caching
- Integrate glslang
- Implement GLSL → SPIRV
- Implement shader caching
- Implement hot reload

**Hour 3**: Vulkan Backend + Integration
- Create VulkanMaterialBackend
- Upload materials to GPU
- Create shader modules
- Test integration

---

## 📝 FILES TO CREATE

### Core Interfaces:
- `core/include/SecretEngine/IMaterialSystem.h`
- `core/include/SecretEngine/IShaderSystem.h`

### MaterialSystem Plugin:
- `plugins/MaterialSystem/CMakeLists.txt`
- `plugins/MaterialSystem/src/MaterialPlugin.h`
- `plugins/MaterialSystem/src/MaterialPlugin.cpp`
- `plugins/MaterialSystem/src/MaterialManager.h`
- `plugins/MaterialSystem/src/MaterialManager.cpp`
- `plugins/MaterialSystem/src/MaterialDatabase.h`
- `plugins/MaterialSystem/src/MaterialDatabase.cpp`
- `plugins/MaterialSystem/src/ShaderCompiler.h`
- `plugins/MaterialSystem/src/ShaderCompiler.cpp`

### Vulkan Backend:
- `plugins/VulkanRenderer/src/VulkanMaterialBackend.h`
- `plugins/VulkanRenderer/src/VulkanMaterialBackend.cpp`
- `plugins/VulkanRenderer/src/VulkanShaderCache.h`
- `plugins/VulkanRenderer/src/VulkanShaderCache.cpp`

---

## ✅ ACCEPTANCE CRITERIA

- [ ] IMaterialSystem interface defined
- [ ] IShaderSystem interface defined
- [ ] MaterialSystem plugin compiles
- [ ] Materials can be created/modified
- [ ] Shaders compile from GLSL
- [ ] Shader caching works
- [ ] Hot reload works (development)
- [ ] VulkanRenderer uses material backend
- [ ] No performance regression
- [ ] All tests pass

---

**Status**: Ready to implement after Phase 1 testing  
**Dependencies**: Phase 1 complete, TextureSystem (Task 2.3)  
**Next**: Implement core interfaces
