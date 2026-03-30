# glTF Integration - Implementation Plan
**Version**: 1.0  
**Date**: 2026-02-02  
**Target**: SecretEngine VulkanRenderer

---

## Overview

This document outlines the step-by-step plan to integrate glTF 2.0 support into SecretEngine's VulkanRenderer plugin, following the modular architecture principles.

---

## Architecture Decision

### ✅ **Chosen Approach**: Single Renderer, Multiple Pipelines

```
VulkanRenderer Plugin
├── Pipeline3D (NEW - for glTF models)
│   ├── PBR shaders
│   ├── Vertex buffers for 3D meshes
│   └── Uniform buffers (camera, model, lighting)
│
├── Pipeline2D (EXISTING - for UI/text)
│   ├── Simple 2D shaders
│   ├── Vertex buffers for quads
│   └── Push constants for positioning
│
└── Shared Resources
    ├── VulkanDevice
    ├── Swapchain
    ├── Command buffers
    └── Render pass
```

**Why this approach?**
- ✅ Matches Unreal Engine's architecture
- ✅ Efficient resource sharing
- ✅ Easy to extend (add more pipelines)
- ✅ Keeps plugin system modular
- ✅ Future-proof for ray tracing, compute, etc.

---

## Implementation Phases

### Phase 1: Foundation (Week 1)
**Goal**: Get basic 3D pipeline working alongside 2D pipeline

#### Task 1.1: Create Pipeline3D Class
```cpp
// File: plugins/VulkanRenderer/src/Pipeline3D.h
class Pipeline3D {
public:
    bool Initialize(VulkanDevice* device, VkRenderPass renderPass);
    void Render(VkCommandBuffer cmd, Camera* camera);
    void Cleanup();

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    
    // Uniform buffers
    VkBuffer m_cameraUBO;
    VkDeviceMemory m_cameraUBOMemory;
};
```

#### Task 1.2: Create Basic 3D Shaders
```glsl
// File: plugins/VulkanRenderer/shaders/basic3d.vert
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(push_constant) uniform ModelPC {
    mat4 model;
} modelPC;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = camera.projection * camera.view * modelPC.model * vec4(inPosition, 1.0);
    fragNormal = mat3(modelPC.model) * inNormal;
    fragTexCoord = inTexCoord;
}
```

```glsl
// File: plugins/VulkanRenderer/shaders/basic3d.frag
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 color = vec3(0.8, 0.8, 0.8) * (0.3 + 0.7 * diff);
    outColor = vec4(color, 1.0);
}
```

#### Task 1.3: Integrate into RendererPlugin
```cpp
// File: plugins/VulkanRenderer/src/RendererPlugin.cpp

void RendererPlugin::InitializeHardware(void* nativeWindow) {
    // ... existing initialization ...
    
    // Create 2D pipeline (existing)
    if (!Create2DPipeline()) {
        logger->LogWarning("2D pipeline creation failed");
    }
    
    // Create 3D pipeline (NEW)
    m_pipeline3D = new Pipeline3D();
    if (!m_pipeline3D->Initialize(m_device, m_renderPass)) {
        logger->LogWarning("3D pipeline creation failed");
    }
}

void RendererPlugin::Present() {
    // ... acquire image ...
    
    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Render 3D scene first
    if (m_pipeline3D) {
        m_pipeline3D->Render(m_commandBuffer, m_camera);
    }
    
    // Render 2D UI on top
    if (m_2dPipeline) {
        DrawWelcomeText(m_commandBuffer);
    }
    
    vkCmdEndRenderPass(m_commandBuffer);
    
    // ... present ...
}
```

---

### Phase 2: glTF Loading (Week 2)
**Goal**: Load glTF files and extract mesh data

#### Task 2.1: Add tinygltf Library
```cmake
# File: plugins/VulkanRenderer/CMakeLists.txt

# Add tinygltf (header-only library)
include(FetchContent)
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.8.13
)
FetchContent_MakeAvailable(tinygltf)

target_link_libraries(VulkanRenderer PRIVATE tinygltf)
```

#### Task 2.2: Create GLTFLoader Class
```cpp
// File: plugins/VulkanRenderer/src/GLTFLoader.h
#include <tiny_gltf.h>

struct Vertex3D {
    float position[3];
    float normal[3];
    float texCoord[2];
};

struct Mesh3D {
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory vertexMemory;
    VkDeviceMemory indexMemory;
};

class GLTFLoader {
public:
    bool LoadModel(const char* filepath, VulkanDevice* device);
    std::vector<Mesh3D>& GetMeshes() { return m_meshes; }

private:
    tinygltf::Model m_model;
    std::vector<Mesh3D> m_meshes;
    
    void ProcessNode(tinygltf::Node& node, VulkanDevice* device);
    void ProcessMesh(tinygltf::Mesh& mesh, VulkanDevice* device);
};
```

#### Task 2.3: Implement Mesh Loading
```cpp
// File: plugins/VulkanRenderer/src/GLTFLoader.cpp

bool GLTFLoader::LoadModel(const char* filepath, VulkanDevice* device) {
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool ret = loader.LoadASCIIFromFile(&m_model, &err, &warn, filepath);
    if (!ret) {
        // Log error
        return false;
    }
    
    // Process all scenes
    for (auto& scene : m_model.scenes) {
        for (int nodeIdx : scene.nodes) {
            ProcessNode(m_model.nodes[nodeIdx], device);
        }
    }
    
    return true;
}

void GLTFLoader::ProcessMesh(tinygltf::Mesh& gltfMesh, VulkanDevice* device) {
    for (auto& primitive : gltfMesh.primitives) {
        Mesh3D mesh;
        
        // Extract positions
        auto& posAccessor = m_model.accessors[primitive.attributes["POSITION"]];
        auto& posBufferView = m_model.bufferViews[posAccessor.bufferView];
        auto& posBuffer = m_model.buffers[posBufferView.buffer];
        
        // Extract normals
        auto& normAccessor = m_model.accessors[primitive.attributes["NORMAL"]];
        // ... similar extraction ...
        
        // Extract tex coords
        auto& texAccessor = m_model.accessors[primitive.attributes["TEXCOORD_0"]];
        // ... similar extraction ...
        
        // Extract indices
        auto& indexAccessor = m_model.accessors[primitive.indices];
        // ... extract indices ...
        
        // Create Vulkan buffers
        CreateVertexBuffer(device, mesh);
        CreateIndexBuffer(device, mesh);
        
        m_meshes.push_back(mesh);
    }
}
```

---

### Phase 3: PBR Materials (Week 3)
**Goal**: Implement physically-based rendering

#### Task 3.1: PBR Shader
```glsl
// File: plugins/VulkanRenderer/shaders/pbr.frag
#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(binding = 1) uniform MaterialUBO {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
} material;

layout(binding = 2) uniform sampler2D baseColorTexture;
layout(binding = 3) uniform sampler2D metallicRoughnessTexture;
layout(binding = 4) uniform sampler2D normalTexture;

layout(location = 0) out vec4 outColor;

// PBR functions
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;
    
    return nom / denom;
}

// ... more PBR functions ...

void main() {
    vec4 baseColor = texture(baseColorTexture, fragTexCoord) * material.baseColorFactor;
    vec2 metallicRoughness = texture(metallicRoughnessTexture, fragTexCoord).bg;
    
    float metallic = metallicRoughness.r * material.metallicFactor;
    float roughness = metallicRoughness.g * material.roughnessFactor;
    
    // PBR lighting calculation
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(cameraPos - fragPosition);
    
    // ... PBR calculations ...
    
    outColor = vec4(finalColor, baseColor.a);
}
```

---

### Phase 4: Scene Management (Week 4)
**Goal**: Manage multiple glTF models in a scene

#### Task 4.1: Create Scene Class
```cpp
// File: plugins/VulkanRenderer/src/Scene.h

struct SceneObject {
    GLTFLoader* model;
    glm::mat4 transform;
    bool visible;
};

class Scene {
public:
    void AddObject(GLTFLoader* model, glm::mat4 transform);
    void RemoveObject(int index);
    void Render(VkCommandBuffer cmd, Pipeline3D* pipeline);
    
private:
    std::vector<SceneObject> m_objects;
};
```

---

## Testing Strategy

### Test 1: Hardcoded Triangle (Phase 1)
```cpp
// Hardcode a simple triangle to test 3D pipeline
std::vector<Vertex3D> vertices = {
    {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
};
```

### Test 2: Simple Cube (Phase 1)
```cpp
// Hardcode a cube to test depth testing
// 8 vertices, 12 triangles (36 indices)
```

### Test 3: Load glTF Cube (Phase 2)
```
// Use Blender to export a simple cube as .gltf
// Test loading and rendering
```

### Test 4: Load glTF with Textures (Phase 3)
```
// Export a textured model from Blender
// Test PBR material rendering
```

---

## File Organization

```
plugins/VulkanRenderer/
├── src/
│   ├── RendererPlugin.cpp         # Main renderer (existing)
│   ├── Pipeline2D.cpp             # 2D/UI pipeline (existing)
│   ├── Pipeline3D.cpp             # NEW: 3D pipeline
│   ├── GLTFLoader.cpp             # NEW: glTF loader
│   ├── Scene.cpp                  # NEW: Scene management
│   └── Camera.cpp                 # NEW: Camera class
│
├── shaders/
│   ├── simple2d.vert/frag         # Existing 2D shaders
│   ├── basic3d.vert/frag          # NEW: Basic 3D shaders
│   └── pbr.vert/frag              # NEW: PBR shaders
│
└── CMakeLists.txt                 # Updated with tinygltf
```

---

## Current Status vs Plan

### ✅ Already Have
- VulkanRenderer plugin structure
- 2D pipeline (text rendering)
- Swapchain, render pass, command buffers
- Android build system

### 🔄 Need to Add
- Pipeline3D class
- GLTFLoader class
- Camera class
- Scene management
- PBR shaders

### 📋 Future Enhancements
- Animation system
- Skeletal animation
- Morph targets
- Multiple lights
- Shadow mapping
- IBL (Image-Based Lighting)

---

## Timeline

| Week | Phase | Deliverable |
|------|-------|-------------|
| 1 | Foundation | 3D pipeline renders hardcoded triangle |
| 2 | glTF Loading | Load and render simple glTF cube |
| 3 | PBR Materials | PBR shaders working with textures |
| 4 | Scene Management | Multiple glTF models in scene |

---

## Next Immediate Steps

1. ✅ Fix current white screen issue (enable 2D text)
2. ✅ Verify 2D pipeline works on Android
3. 🔄 Create Pipeline3D class
4. 🔄 Render hardcoded 3D triangle
5. 🔄 Add tinygltf library
6. 🔄 Load simple glTF file

---

## Conclusion

This plan follows industry best practices:
- ✅ Single renderer, multiple pipelines (like Unreal)
- ✅ Modular architecture (unique to SecretEngine)
- ✅ Incremental development (test each phase)
- ✅ Future-proof for advanced features

**The architecture is sound. Let's build it step by step!**

---

**Document Version**: 1.0  
**Last Updated**: 2026-02-02  
**Status**: Ready for Implementation ✅
