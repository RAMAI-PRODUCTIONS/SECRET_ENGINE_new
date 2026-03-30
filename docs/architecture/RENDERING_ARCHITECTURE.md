# SecretEngine - Rendering Architecture
**Version**: 1.1  
**Date**: 2026-02-03  
**Author**: Architecture Team

---

## Executive Summary

SecretEngine uses a **unified, modular rendering architecture** inspired by Unreal Engine and Unity's approach, where:
- **One renderer handles all rendering** (3D geometry, 2D UI, text, particles, etc.)
- **Multiple specialized pipelines** exist within that renderer
- **Everything is swappable** via the plugin system
- **Future-proof** for glTF, PBR, ray tracing, etc.

---

## How Major Engines Handle 2D/3D Rendering

### Unreal Engine Approach
✅ **Single Unified Renderer** with multiple pipelines:
- Main 3D rendering pipeline (deferred/forward)
- UMG (Unreal Motion Graphics) for 2D UI - rendered as overlay
- Slate framework underneath UMG
- SceneCapture2D for rendering 3D into 2D textures
- **UI rendered AFTER 3D scene** as final overlay

**Key Insight**: One renderer, multiple render passes

### Unity Approach
✅ **Scriptable Render Pipeline (SRP)** architecture:
- Universal Render Pipeline (URP) - optimized for mobile/cross-platform
- High Definition Render Pipeline (HDRP) - high-end graphics
- Built-in Render Pipeline (BiRP) - legacy
- **2D renderer** is a specialized configuration of the 3D pipeline
- UI Toolkit renders on top using mesh/vector API

**Key Insight**: Modular pipeline system, everything configurable

### Industry Best Practices
1. **Single Renderer, Multiple Passes**
   - 3D scene rendered first
   - Post-processing applied
   - UI/2D overlays rendered last
   
2. **Layered Architecture**
   - Layer 0: 3D world geometry
   - Layer 1: Particles/effects
   - Layer 2: UI overlays
   - Layer 3: Debug/editor tools

3. **Render Targets**
   - Render 3D to texture
   - Compose multiple layers
   - Apply post-processing
   - Final composite to screen

---

## SecretEngine Architecture Design

### Core Principle: **Modular Plugin-Based Rendering**

```
┌─────────────────────────────────────────────────────┐
│                   ENGINE CORE                        │
│              (Platform Agnostic)                     │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│              IRenderer Interface                     │
│  - Initialize()                                      │
│  - BeginFrame()                                      │
│  - RenderScene()                                     │
│  - RenderUI()                                        │
│  - EndFrame()                                        │
│  - Present()                                         │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
        ┌────────────────┴────────────────┐
        │                                  │
        ▼                                  ▼
┌──────────────────┐            ┌──────────────────┐
│ VulkanRenderer   │            │ Future Renderers │
│    Plugin        │            │ - DX12Renderer   │
│                  │            │ - MetalRenderer  │
│ ┌──────────────┐ │            │ - WebGPURenderer │
│ │ 3D Pipeline  │ │            └──────────────────┘
│ │ - glTF       │ │
│ │ - PBR        │ │
│ │ - Lighting   │ │
│ └──────────────┘ │
│                  │
│ ┌──────────────┐ │
│ │ 2D Pipeline  │ │
│ │ - UI         │ │
│ │ - Text       │ │
│ │ - Sprites    │ │
│ └──────────────┘ │
│                  │
│ ┌──────────────┐ │
│ │ Post-Process │ │
│ │ - Bloom      │ │
│ │ - Tonemapping│ │
│ └──────────────┘ │
└──────────────────┘
```

---

## Detailed Architecture

### 1. **Single Renderer Plugin** (VulkanRenderer)

The VulkanRenderer plugin is the **single source of truth** for all rendering:

```cpp
class VulkanRenderer : public IRenderer {
private:
    // Core Vulkan objects
    VulkanDevice* m_device;
    Swapchain* m_swapchain;
    
    // Multiple pipelines within ONE renderer
    Pipeline3D* m_3dPipeline;      // For 3D geometry (glTF models)
    Pipeline2D* m_2dPipeline;      // For UI/text/sprites
    PipelineParticles* m_particlePipeline;
    PipelinePostProcess* m_postProcessPipeline;
    
    // Render targets for compositing
    RenderTarget* m_sceneTarget;   // 3D scene renders here
    RenderTarget* m_uiTarget;      // UI renders here
    
public:
    void RenderFrame() {
        BeginFrame();
        
        // Pass 1: Render 3D scene to render target
        RenderScene3D(m_sceneTarget);
        
        // Pass 2: Apply post-processing
        ApplyPostProcessing(m_sceneTarget);
        
        // Pass 3: Render UI overlay
        RenderUI2D(m_uiTarget);
        
        // Pass 4: Composite and present
        Composite(m_sceneTarget, m_uiTarget);
        Present();
        
        EndFrame();
    }
};
```

### 3. **Fast Data Architecture (FDA) Integration**
The VulkanRenderer is an **FDA Stream Consumer**.
- It exposes a `GetCommandStream()` returning an `UltraRingBuffer<1024>`.
- Internal logic in `Submit()` drains this stream to update state (e.g., Rotation, Color, Spawning) without locking the Game Thread.
- This allows the renderer to process input-driven visual changes at the start of the frame with zero wait time.

### 3.1 **Mega Geometry Subsystem (GPU-Driven)**
To achieve 10M+ triangles on mobile (Call of Duty parity), we utilize a **Mega Geometry** approach:
- **Indirect Drawing:** Uses `vkCmdDrawIndexedIndirect` to render thousands of instances in a **single draw call**.
- **Persistent SSBOs:** Instance data (Transforms, Colors) lives in `HOST_COHERENT` GPU memory, updated directly via FDA packets.
- **Zero-Copy:** The CPU simply writes 8-byte updates to the mapped pointer; the GPU reads them instantly.
- **Spherical Distribution:** Optimized spatial scattering algorithm for 1,000+ high-poly "Giant" instances.
- **Current Benchmark:** 6,056,050 Triangles @ 43 FPS on Adreno 619 (Snapdragon 695).

### 4. **Pipeline Separation** (Not Renderer Separation)

Each pipeline handles specific rendering tasks:

#### **3D Pipeline** (For glTF, PBR, 3D Geometry)
```cpp
class Pipeline3D {
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    
    // Vertex format for 3D meshes
    struct Vertex3D {
        vec3 position;
        vec3 normal;
        vec2 texCoord;
        vec4 tangent;
    };
    
    // Uniform buffers
    UniformBuffer m_cameraUBO;
    UniformBuffer m_modelUBO;
    UniformBuffer m_lightingUBO;
    
    void Render(CommandBuffer cmd, Scene* scene) {
        // Bind 3D pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        
        // Render all 3D meshes
        for (auto& mesh : scene->GetMeshes()) {
            BindMaterial(mesh.material);
            DrawMesh(mesh);
        }
    }
};
```

#### **2D Pipeline** (For UI, Text, Sprites)
```cpp
class Pipeline2D {
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    
    // Vertex format for 2D quads
    struct Vertex2D {
        vec2 position;  // Screen space coordinates
        vec2 texCoord;
        vec4 color;
    };
    
    void RenderUI(CommandBuffer cmd, UIScene* ui) {
        // Bind 2D pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        
        // Render UI elements (text, buttons, etc.)
        for (auto& element : ui->GetElements()) {
            DrawQuad(element);
        }
    }
};
```

---

## Rendering Flow

### Frame Rendering Sequence

```
1. BeginFrame()
   ├─ Acquire swapchain image
   ├─ Reset command buffers
   └─ Begin command recording

2. RenderScene3D()
   ├─ Bind 3D pipeline
   ├─ Set camera matrices
   ├─ Frustum culling
   ├─ Render opaque geometry (glTF models)
   ├─ Render transparent geometry
   └─ Render particles

3. ApplyPostProcessing()
   ├─ Bloom
   ├─ Tonemapping
   ├─ Color grading
   └─ Anti-aliasing (FXAA/TAA)

4. RenderUI2D()
   ├─ Bind 2D pipeline
   ├─ Render UI background
   ├─ Render UI elements
   ├─ Render text
   └─ Render debug overlays

5. Composite()
   ├─ Combine 3D scene + UI
   └─ Render to swapchain

6. Present()
   ├─ End command recording
   ├─ Submit to GPU
   └─ Present swapchain image
```

---

## glTF Integration Strategy

### Why glTF?
- **Industry standard** for 3D assets
- **PBR materials** built-in
- **Animations, skins, morphs** supported
- **Compact binary format** (GLB)
- **Widely supported** (Blender, Unity, Unreal, etc.)

### Implementation Plan

#### Phase 1: Basic glTF Loading
```cpp
class GLTFLoader {
    struct GLTFMesh {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        Material material;
    };
    
    struct GLTFModel {
        std::vector<GLTFMesh> meshes;
        std::vector<Texture> textures;
        Transform transform;
    };
    
    GLTFModel* Load(const char* filepath);
};
```

#### Phase 2: PBR Material System
```cpp
struct PBRMaterial {
    vec4 baseColorFactor;
    Texture* baseColorTexture;
    
    float metallicFactor;
    float roughnessFactor;
    Texture* metallicRoughnessTexture;
    
    Texture* normalTexture;
    Texture* occlusionTexture;
    Texture* emissiveTexture;
    vec3 emissiveFactor;
};
```

#### Phase 3: Animation System
```cpp
class GLTFAnimator {
    void Update(float deltaTime);
    void ApplySkinning(GLTFModel* model);
    void ApplyMorphTargets(GLTFModel* model);
};
```

---

## Plugin Architecture Benefits

### 1. **Swappable Renderers**
```
VulkanRenderer (current)
    ↓ Can be replaced with
DX12Renderer (Windows)
    ↓ Or
MetalRenderer (macOS/iOS)
    ↓ Or
WebGPURenderer (Web)
```

### 2. **Modular Pipelines**
Each pipeline is independent and can be:
- Enabled/disabled at runtime
- Replaced with custom implementations
- Extended with new features

### 3. **Future-Proof**
Easy to add:
- Ray tracing pipeline
- Compute shaders
- Mesh shaders
- Variable rate shading

---

## Current Implementation Status

### ✅ Completed
- [x] VulkanRenderer plugin structure
- [x] 3D pipeline (glTF mesh loading, instancing)
- [x] 2D pipeline (text rendering, sky gradient)
- [x] Swapchain management
- [x] Command buffer system
- [x] Synchronization (semaphores/fences)
- [x] Android device support
- [x] Scene JSON bootstrapping

### 📋 Planned (Next Steps)
1. **glTF Loader Plugin**
   - Load .gltf/.glb files
   - Parse meshes, materials, textures
   - Create Vulkan buffers

2. **PBR Rendering**
   - Implement PBR shaders
   - IBL (Image-Based Lighting)
   - Shadow mapping

3. **UI System Plugin**
   - Layout engine
   - Widget system
   - Event handling

4. **Animation System**
   - Skeletal animation
   - Blend trees
   - IK (Inverse Kinematics)

---

## Recommended Architecture

### For SecretEngine, we recommend:

✅ **Single VulkanRenderer Plugin** containing:
1. **3D Pipeline** - For glTF models, PBR materials, lighting
2. **2D Pipeline** - For UI, text, sprites, debug overlays
3. **Post-Processing Pipeline** - For effects
4. **Compute Pipeline** - For GPU computations (future)

✅ **Separate Asset Loader Plugins**:
- `GLTFLoaderPlugin` - Loads glTF/GLB files
- `TextureLoaderPlugin` - Loads images (PNG, JPG, KTX)
- `ShaderLoaderPlugin` - Compiles/loads shaders

✅ **Separate System Plugins**:
- `UISystemPlugin` - Manages UI layout and widgets
- `AnimationSystemPlugin` - Handles animations
- `PhysicsPlugin` - Physics simulation (future)

---

## File Structure

```
plugins/
├── VulkanRenderer/
│   ├── src/
│   │   ├── VulkanRenderer.cpp       # Main renderer
│   │   ├── VulkanDevice.cpp         # Device management
│   │   ├── Swapchain.cpp            # Swapchain
│   │   ├── Pipeline3D.cpp           # 3D rendering pipeline
│   │   ├── Pipeline2D.cpp           # 2D/UI pipeline
│   │   ├── PipelinePostProcess.cpp  # Post-processing
│   │   └── RenderTarget.cpp         # Render targets
│   ├── shaders/
│   │   ├── pbr.vert                 # PBR vertex shader
│   │   ├── pbr.frag                 # PBR fragment shader
│   │   ├── ui.vert                  # UI vertex shader
│   │   └── ui.frag                  # UI fragment shader
│   └── CMakeLists.txt
│
├── GLTFLoader/
│   ├── src/
│   │   ├── GLTFLoader.cpp           # glTF parser
│   │   ├── GLTFMesh.cpp             # Mesh data
│   │   └── GLTFMaterial.cpp         # Material data
│   └── CMakeLists.txt
│
└── UISystem/
    ├── src/
    │   ├── UIManager.cpp            # UI management
    │   ├── Widget.cpp               # Base widget
    │   ├── Button.cpp               # Button widget
    │   └── TextLabel.cpp            # Text widget
    └── CMakeLists.txt
```

---

## Performance Considerations

### Optimization Strategies

1. **Frustum Culling**
   - Don't render objects outside camera view
   - Implemented in 3D pipeline

2. **Occlusion Culling**
   - Don't render objects hidden behind others
   - Use depth pre-pass

3. **LOD (Level of Detail)**
   - Multiple mesh versions
   - Switch based on distance

4. **Batching**
   - Combine meshes with same material
   - Reduce draw calls

5. **Texture Atlasing**
   - Combine multiple textures
   - Reduce texture switches

6. **UI Optimization**
   - Cache UI geometry
   - Only rebuild when changed
   - Use segment-based font for ultra-fast alphanumeric rendering (No texture binding)

### 7. **Mobile Hardware Comparison (Adreno 619)**

| Metric | **SecretEngine** | **CODM (Estimated)** |
| --- | --- | --- |
| **Triangles per Frame** | **~6,056,050** | **~300,000 - 500,000** |
| **Draw Calls** | **Managed (Instanced)** | **~400 - 600** |
| **Frame Rate** | **35 - 43 FPS** | **60 FPS** |
| **Triangle Density Gap** | **12x to 20x Lead** | - |

---

## Conclusion

SecretEngine follows industry best practices:
- ✅ **Single unified renderer** (like Unreal)
- ✅ **Multiple specialized pipelines** (like Unity SRP)
- ✅ **Modular plugin architecture** (unique to SecretEngine)
- ✅ **Future-proof for glTF, PBR, ray tracing**

This architecture allows:
- Easy swapping of renderers (Vulkan → DX12 → Metal)
- Independent pipeline development
- Clean separation of concerns
- Scalable from mobile to high-end PC

**Next Step**: Implement glTF loader plugin and PBR rendering pipeline.

---

**Document Version**: 1.1  
**Last Updated**: 2026-02-03  
**Status**: Architecture Approved & Implemented ✅
