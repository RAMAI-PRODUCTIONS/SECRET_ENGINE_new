# Quick Reference: Rendering Architecture
**For**: SecretEngine Developers  
**Updated**: 2026-02-02

---

## TL;DR

✅ **Use ONE renderer with MULTIPLE pipelines**  
❌ **Don't create separate renderers for 2D and 3D**

---

## Current Architecture

```
VulkanRenderer Plugin (ONE RENDERER)
├── Pipeline2D ✅ (EXISTING - for UI/text/sky)
└── Pipeline3D ✅ (EXISTING - for glTF/3D models)
```

---

## How It Works

### Rendering Flow
```
1. BeginFrame()
2. Render 3D Scene (Pipeline3D)
3. Render UI Overlay (Pipeline2D)
4. Present()
```

### Code Example
```cpp
void VulkanRenderer::RenderFrame() {
    BeginFrame();
    
    // Render 3D geometry first
    if (m_pipeline3D) {
        m_pipeline3D->Render(m_commandBuffer, m_camera);
    }
    
    // Render 2D UI on top
    if (m_pipeline2D) {
        m_pipeline2D->Render(m_commandBuffer);
    }
    
    Present();
}
```

---

## Key Principles

### 1. Single Renderer
```cpp
// ✅ CORRECT
class VulkanRenderer : public IRenderer {
    Pipeline3D* m_pipeline3D;
    Pipeline2D* m_pipeline2D;
};

// ❌ WRONG
class VulkanRenderer3D : public IRenderer { };
class VulkanRenderer2D : public IRenderer { };
```

### 2. Shared Resources
```cpp
// All pipelines share:
VulkanDevice* m_device;        // One device
Swapchain* m_swapchain;        // One swapchain
VkCommandBuffer m_commandBuffer; // Shared command buffer
VkRenderPass m_renderPass;     // Shared render pass
```

### 3. Independent Pipelines
```cpp
// Each pipeline is self-contained
class Pipeline3D {
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    void Render(VkCommandBuffer cmd);
};

class Pipeline2D {
    VkPipeline m_pipeline;
    VkPipelineLayout m_layout;
    void Render(VkCommandBuffer cmd);
};
```

---

## What This Means for glTF

### Implementation Strategy
```
Step 1: Create Pipeline3D class
Step 2: Add basic 3D shaders
Step 3: Render hardcoded triangle
Step 4: Add tinygltf library
Step 5: Load glTF files
Step 6: Render glTF meshes
```

### File Structure
```
plugins/VulkanRenderer/
├── src/
│   ├── RendererPlugin.cpp     # Main renderer
│   ├── Pipeline2D.cpp         # UI pipeline (existing)
│   ├── Pipeline3D.cpp         # 3D pipeline (NEW)
│   └── GLTFLoader.cpp         # glTF loader (NEW)
└── shaders/
    ├── simple2d.vert/frag     # 2D shaders (existing)
    └── pbr.vert/frag          # 3D PBR shaders (NEW)
```

---

## Common Questions

### Q: Do I need two renderers for 2D and 3D?
**A**: No! Use one renderer with two pipelines.

### Q: How do I render UI on top of 3D?
**A**: Render 3D first, then render 2D in the same render pass.

### Q: Can I swap renderers?
**A**: Yes! VulkanRenderer → DX12Renderer → MetalRenderer (same interface).

### Q: How do I add glTF support?
**A**: Create Pipeline3D class and GLTFLoader class within VulkanRenderer plugin.

### Q: Is this how Unreal/Unity do it?
**A**: Yes! This is industry standard.

---

## Next Steps

### This Week
1. ✅ Fix white screen (enable 2D text)
2. ✅ Document architecture
3. ✅ Pipeline3D implementation
4. ✅ Sky Gradient & Touch Input
5. ✅ Scene JSON Bootstrapping

### Next Week
1. Multi-mesh instance optimization
2. Advanced PBR Material support
3. Physics Integration (PhysX/Jolt)

---

## Resources

- `docs/architecture/RENDERING_ARCHITECTURE.md` - Full spec
- `docs/architecture/GLTF_INTEGRATION_PLAN.md` - Implementation plan
- `docs/architecture/ARCHITECTURE_RESEARCH_SUMMARY.md` - Research findings

---

## Visual Reference

See generated diagrams:
- `rendering_architecture_diagram.png` - Pipeline architecture
- `engine_comparison_diagram.png` - Comparison with Unreal/Unity

---

**Remember**: One renderer, multiple pipelines. That's the way! 🚀
