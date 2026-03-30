# Architecture Research Summary
**Date**: 2026-02-02  
**Topic**: How Major Game Engines Handle 2D/3D Rendering

---

## Key Question
> "How does Unreal have 2D and 3D? Are there two different renderers or one single renderer handling both UI and 3D geometry?"

---

## Answer: **Single Renderer, Multiple Pipelines**

All major game engines (Unreal, Unity, Godot) use **ONE renderer** with **multiple specialized pipelines**.

---

## Industry Approaches

### 🎮 Unreal Engine
**Architecture**: Single unified renderer with specialized systems

```
Unreal Rendering Engine
├── Main 3D Pipeline (Deferred/Forward)
│   ├── Geometry rendering
│   ├── Lighting (dynamic/static)
│   ├── Post-processing
│   └── Ray tracing (optional)
│
├── UMG (Unreal Motion Graphics)
│   ├── Built on Slate framework
│   ├── Renders AFTER 3D scene
│   └── Overlaid on final image
│
└── SceneCapture2D
    └── Renders 3D to 2D texture
```

**Key Insight**: 
- 3D scene renders first
- UI renders as overlay on top
- Same renderer, different render passes

---

### 🎯 Unity
**Architecture**: Scriptable Render Pipeline (SRP)

```
Unity Rendering System
├── Universal Render Pipeline (URP)
│   ├── Optimized for mobile/cross-platform
│   ├── 2D renderer (specialized config of 3D)
│   └── UI Toolkit (mesh/vector API)
│
├── High Definition Render Pipeline (HDRP)
│   ├── High-end graphics
│   ├── Physically-based rendering
│   └── Advanced lighting
│
└── Built-in Render Pipeline (BiRP)
    └── Legacy system
```

**Key Insight**:
- 2D is just a specialized 3D pipeline
- UI rendered on top using same system
- Modular, swappable pipelines

---

## Best Practices Summary

### 1. **Rendering Order**
```
Frame Rendering Sequence:
1. Clear screen
2. Render 3D scene → Render Target
3. Apply post-processing
4. Render UI overlay → Screen
5. Present to display
```

### 2. **Layered Architecture**
```
Layer 0: 3D World Geometry
Layer 1: Particles & Effects
Layer 2: UI Overlays
Layer 3: Debug/Editor Tools
```

### 3. **Optimization Techniques**

#### For 3D:
- **Frustum Culling**: Don't render what's off-screen
- **Occlusion Culling**: Don't render hidden objects
- **LOD (Level of Detail)**: Lower detail for distant objects
- **Batching**: Combine meshes with same material
- **Texture Atlasing**: Combine textures to reduce draw calls

#### For UI:
- **Event-Driven Updates**: Only redraw when changed
- **Widget Caching**: Cache static UI elements
- **Texture-Based UI**: Use textures instead of complex materials
- **Separate Canvas**: UI on separate render target

---

## SecretEngine Architecture Decision

### ✅ **Chosen Approach**: Single Renderer, Multiple Pipelines

```
VulkanRenderer Plugin (ONE RENDERER)
│
├── Pipeline3D
│   ├── For glTF models
│   ├── PBR materials
│   ├── Lighting & shadows
│   └── Animations
│
├── Pipeline2D
│   ├── For UI elements
│   ├── Text rendering
│   ├── Sprites
│   └── Debug overlays
│
├── PipelineParticles (Future)
│   └── Particle effects
│
└── PipelinePostProcess (Future)
    ├── Bloom
    ├── Tonemapping
    └── Anti-aliasing
```

---

## Why This Architecture?

### ✅ Advantages

1. **Efficient Resource Sharing**
   - One VulkanDevice
   - One Swapchain
   - Shared command buffers
   - Shared descriptor pools

2. **Clear Separation of Concerns**
   - 3D pipeline handles 3D geometry
   - 2D pipeline handles UI
   - Each can be optimized independently

3. **Modular & Swappable**
   ```
   VulkanRenderer → DX12Renderer → MetalRenderer
   (Same interface, different implementation)
   ```

4. **Future-Proof**
   - Easy to add ray tracing pipeline
   - Easy to add compute pipeline
   - Easy to add mesh shaders

5. **Matches Industry Standards**
   - Same approach as Unreal Engine
   - Same approach as Unity SRP
   - Proven architecture

---

## Implementation Strategy

### Phase 1: Current (Week 1)
```
✅ VulkanRenderer plugin exists
✅ Pipeline2D exists (text rendering)
🔄 Fix white screen issue
🔄 Verify 2D works on Android
```

### Phase 2: Add 3D Pipeline (Week 2-3)
```
📋 Create Pipeline3D class
📋 Basic 3D shaders
📋 Render hardcoded triangle
📋 Render hardcoded cube
```

### Phase 3: glTF Integration (Week 4-5)
```
📋 Add tinygltf library
📋 GLTFLoader class
📋 Load simple glTF model
📋 Render glTF meshes
```

### Phase 4: PBR Materials (Week 6-7)
```
📋 PBR shaders
📋 Material system
📋 Texture loading
📋 Lighting system
```

---

## Comparison with Other Engines

| Feature | Unreal | Unity | SecretEngine |
|---------|--------|-------|--------------|
| **Architecture** | Single renderer | SRP (modular) | Single renderer |
| **2D/3D** | Same renderer | Same renderer | Same renderer |
| **UI System** | UMG/Slate | UI Toolkit | Pipeline2D |
| **3D Rendering** | Deferred/Forward | URP/HDRP | Pipeline3D |
| **Swappable** | No | Yes (SRP) | Yes (plugins) |
| **glTF Support** | Via plugins | Built-in | Planned |

---

## Key Takeaways

### 1. **Never Use Two Separate Renderers**
❌ Bad: VulkanRenderer3D + VulkanRenderer2D  
✅ Good: VulkanRenderer with Pipeline3D + Pipeline2D

### 2. **Render in Layers**
```
1. 3D Scene → Render Target
2. Post-Processing → Render Target
3. UI Overlay → Screen
4. Present
```

### 3. **Share Resources**
- One Vulkan instance
- One device
- One swapchain
- Multiple pipelines

### 4. **Keep Pipelines Independent**
- 3D pipeline doesn't know about 2D
- 2D pipeline doesn't know about 3D
- Renderer orchestrates both

---

## Recommended Reading

1. **Unreal Engine Documentation**
   - Rendering Architecture
   - UMG Best Practices
   - SceneCapture2D

2. **Unity Documentation**
   - Scriptable Render Pipeline
   - Universal Render Pipeline
   - UI Toolkit

3. **Vulkan Best Practices**
   - Render Pass Design
   - Pipeline Management
   - Descriptor Sets

---

## Next Steps for SecretEngine

### Immediate (This Week)
1. ✅ Fix white screen (enable 2D text)
2. ✅ Document architecture decisions
3. 🔄 Test 2D pipeline on Android

### Short Term (Next 2 Weeks)
1. Create Pipeline3D class
2. Implement basic 3D shaders
3. Render hardcoded 3D geometry

### Medium Term (Next Month)
1. Integrate tinygltf library
2. Load glTF models
3. Implement PBR rendering

### Long Term (Next Quarter)
1. Animation system
2. Advanced lighting
3. Shadow mapping
4. Ray tracing (optional)

---

## Conclusion

**SecretEngine's architecture is sound and follows industry best practices.**

✅ Single VulkanRenderer plugin  
✅ Multiple specialized pipelines  
✅ Modular and swappable  
✅ Future-proof for glTF, PBR, ray tracing  

**This is the correct approach. Let's build it!**

---

**Document Version**: 1.0  
**Author**: Architecture Team  
**Status**: Approved ✅  
**Last Updated**: 2026-02-02
