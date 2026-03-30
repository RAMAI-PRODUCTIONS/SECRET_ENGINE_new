# TEXTURE SYSTEM INTEGRATION GUIDE
## GPU-Optimized ASTC Texture Support for SecretEngine

---

## 🎯 OVERVIEW

This texture system provides:
- **ASTC Compression**: Hardware-accelerated on mobile & modern GPUs (2-8 bpp)
- **Bindless Textures**: Up to 16,384 simultaneous textures
- **Zero-Copy Uploads**: Direct staging buffer to GPU
- **Automatic Format Selection**: Falls back to BC7/RGBA8 if ASTC unavailable
- **Persistent Mapping**: Minimal CPU overhead

---

## 📦 FILES ADDED

### Core System
```
TextureManager.h          - Main texture management API
TextureManager.cpp        - Implementation with ASTC encoding
MegaGeometryRendererTextured.h  - Extended renderer with textures
mega_geometry_textured.vert     - Updated vertex shader
mega_geometry_textured.frag     - Bindless texture sampling shader
```

### Utilities
```
ASTCConverter.h          - Batch PNG→ASTC conversion tool
```

### Dependencies (Include these headers)
```
stb_image.h              - PNG decoding (single header)
astcenc.h                - ASTC encoding (single header)
```

---

## 🚀 QUICK START

### 1. Initialize Texture Manager

```cpp
// In RendererPlugin::OnLoad()
#include "TextureManager.h"

SecretEngine::Textures::TextureManager* m_textureManager;

bool RendererPlugin::OnLoad(SecretEngine::ICore* core) {
    // ... existing initialization ...
    
    // Create texture manager
    m_textureManager = new SecretEngine::Textures::TextureManager();
    if (!m_textureManager->Initialize(m_device, core)) {
        core->GetLogger()->LogError("Renderer", "Failed to init textures");
        return false;
    }
    
    return true;
}
```

### 2. Update MegaGeometryRenderer Initialization

```cpp
// Initialize with texture manager
m_megaGeometry = new SecretEngine::MegaGeometry::MegaGeometryRendererTextured();
m_megaGeometry->Initialize(m_device, m_renderPass, m_core, m_textureManager);
```

### 3. Load Textures

```cpp
// Load textures (auto-converts PNG to ASTC)
uint32_t brickTexture = m_textureManager->LoadTexture("textures/brick_wall.png");
uint32_t grassTexture = m_textureManager->LoadTexture("textures/grass.png", 
    SecretEngine::Textures::ASTCFormat::ASTC_8x8_UNORM);
```

### 4. Create Textured Instances

```cpp
// Load mesh
m_megaGeometry->LoadMesh("meshes/cube.mesh", 0);

// Create instances with textures
uint32_t instance1 = m_megaGeometry->AddInstance(0, 0.0f, 0.0f, 0.0f, brickTexture);
uint32_t instance2 = m_megaGeometry->AddInstance(0, 5.0f, 0.0f, 0.0f, grassTexture);

// Update texture dynamically
m_megaGeometry->UpdateInstanceTexture(instance1, grassTexture);
```

### 5. Render with Textures

```cpp
void RendererPlugin::OnUpdate(float dt) {
    // ... existing render code ...
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Render textured geometry
    m_megaGeometry->PreRender(cmd);  // GPU culling
    m_megaGeometry->Render(cmd);     // Draw with textures
    
    vkCmdEndRenderPass(cmd);
}
```

---

## 🎨 ADVANCED USAGE

### Texture Atlas (Batch Multiple Textures)

```cpp
// Define textures to pack into atlas
SecretEngine::Textures::TextureManager::AtlasTextureInfo atlasTextures[] = {
    {"textures/player_idle.png", 0},
    {"textures/player_walk1.png", 0},
    {"textures/player_walk2.png", 0},
    {"textures/enemy_idle.png", 0}
};

// Create 4096x4096 atlas
uint32_t atlasID = m_textureManager->CreateAtlas(atlasTextures, 4, 4096);

// atlasTextures[i].slotIndex now contains the texture slot for each sprite
uint32_t playerIdleSlot = atlasTextures[0].slotIndex;
```

### Material System

```cpp
// Create PBR material
SecretEngine::Textures::TextureManager::Material mat = {};
mat.albedoTexture = m_textureManager->LoadTexture("textures/wood_albedo.png");
mat.normalTexture = m_textureManager->LoadTexture("textures/wood_normal.png");
mat.metallicRoughnessTexture = m_textureManager->LoadTexture("textures/wood_mr.png");
mat.albedoFactor[0] = 1.0f;
mat.albedoFactor[1] = 1.0f;
mat.albedoFactor[2] = 1.0f;
mat.albedoFactor[3] = 1.0f;
mat.metallicFactor = 0.2f;
mat.roughnessFactor = 0.8f;

uint32_t materialID = m_textureManager->CreateMaterial(mat);
```

### Batch PNG Conversion (Offline Tool)

```cpp
#include "ASTCConverter.h"

// Setup compression params
SecretEngine::Tools::ASTCConversionParams params;
params.blockSize = SecretEngine::Tools::ASTCBlockSize::BLOCK_8x8;
params.quality = SecretEngine::Tools::ASTCQuality::MEDIUM;
params.generateMipmaps = true;

// Convert entire directory
SecretEngine::Tools::ASTCConverter::ConvertDirectory(
    "assets/textures_source",
    "assets/textures_astc",
    params
);
```

---

## 🔧 SHADER UPDATES

### Vertex Shader Changes

The vertex shader now outputs texture coordinates and texture IDs:

```glsl
layout(location = 0) out vec2 fragTexCoord;  // UV coordinates
layout(location = 3) out flat uint fragTextureID;  // Bindless texture index

// Instance data now includes textureID
struct InstanceData {
    vec4 row0, row1, row2;
    uint packedColor;
    uint textureID;  // NEW!
    uint _padding0, _padding1;
};
```

### Fragment Shader Changes

Bindless texture sampling:

```glsl
// Bindless texture array (up to 16,384 textures)
layout(binding = 1) uniform sampler2D textures[16384];

void main() {
    vec4 texColor = texture(textures[nonuniformEXT(fragTextureID)], fragTexCoord);
    outColor = fragColor * texColor;
}
```

---

## 📊 PERFORMANCE

### ASTC Compression Benefits

| Format | Bits/Pixel | Quality | Mobile GPU | Desktop GPU |
|--------|------------|---------|------------|-------------|
| ASTC 4x4 | 8.00 | Excellent | ✅ Native | ✅ Supported |
| ASTC 8x8 | 2.00 | Good | ✅ Native | ✅ Supported |
| BC7 | 8.00 | Excellent | ❌ No | ✅ Native |
| RGBA8 | 32.00 | Perfect | ✅ Yes | ✅ Yes |

### Memory Savings Example

4096x4096 texture:
- **RGBA8**: 64 MB
- **ASTC 8x8**: 16 MB (4x reduction)
- **ASTC 12x12**: 11.4 MB (5.6x reduction)

### GPU Performance

- **Texture Cache**: ASTC reduces bandwidth by 4-8x
- **Bindless Access**: No descriptor set switching overhead
- **Mipmap Support**: Full LOD chain for optimal sampling

---

## 🛠️ INSTANCE DATA LAYOUT

```cpp
// 64-byte aligned structure (cache-friendly)
struct InstanceDataTextured {
    Matrix3x4 transform;    // 48 bytes - 3x4 row-major matrix
    uint32_t packedColor;   // 4 bytes  - RGBA8 packed
    uint32_t textureID;     // 4 bytes  - Bindless index
    uint32_t _padding0;     // 4 bytes  - Alignment
    uint32_t _padding1;     // 4 bytes  - Alignment
};
```

---

## 🔍 DEBUGGING

### Check ASTC Support

```cpp
VkFormatProperties props;
vkGetPhysicalDeviceFormatProperties(physicalDevice, 
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK, &props);

if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
    printf("ASTC 8x8 supported!\n");
} else {
    printf("Falling back to BC7/RGBA8\n");
}
```

### Texture Stats

```cpp
// Get VRAM usage
uint64_t vram = m_textureManager->GetVRAMUsage();
printf("Textures using %.2f MB VRAM\n", vram / (1024.0 * 1024.0));

// Get texture count
uint32_t count = m_textureManager->GetTextureCount();
printf("Loaded %u textures\n", count);
```

### Render Stats

```cpp
auto stats = m_megaGeometry->GetStats();
printf("Instances: %u, Triangles: %u, Draw Calls: %u\n", 
    stats.totalInstances, stats.totalTriangles, stats.drawCalls);
```

---

## 📋 PIPELINE DESCRIPTOR SET LAYOUT

```cpp
// Binding 0: Instance SSBO (compute + vertex shader)
layout(std430, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// Binding 1: Bindless texture array (fragment shader)
layout(binding = 1) uniform sampler2D textures[16384];
```

Update descriptor set creation to include texture binding:

```cpp
VkDescriptorSetLayoutBinding bindings[2] = {
    {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16384, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
};
```

---

## ⚠️ IMPORTANT NOTES

1. **ASTC Requires GPU Support**: Check format support at runtime
2. **Bindless Requires VK_EXT_descriptor_indexing**: Enable device extension
3. **Maximum Texture Array Size**: Limited to 16,384 (GPU dependent)
4. **Staging Buffer Size**: 64MB limit per upload (split large textures)
5. **Double Buffering**: System uses 2-frame latency for smooth updates

---

## 🎯 INTEGRATION CHECKLIST

- [ ] Copy `TextureManager.h/cpp` to project
- [ ] Copy `MegaGeometryRendererTextured.h` to project  
- [ ] Copy updated shader files (`.vert`, `.frag`)
- [ ] Include `stb_image.h` for PNG loading
- [ ] Include `astcenc.h` for ASTC compression
- [ ] Enable `VK_EXT_descriptor_indexing` device extension
- [ ] Update pipeline to use new shaders
- [ ] Initialize `TextureManager` before `MegaGeometryRenderer`
- [ ] Update instance data structure to 64 bytes
- [ ] Compile shaders with `glslangValidator` or `glslc`

---

## 📞 SUPPORT

For issues or questions about the texture system, check:
1. GPU ASTC format support
2. Descriptor indexing extension enabled
3. Shader compilation with correct SPIR-V version
4. Instance data alignment (must be 64 bytes)

---

**Happy Texturing!** 🎨
