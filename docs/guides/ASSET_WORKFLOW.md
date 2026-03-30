# SecretEngine: Asset Workflow & Scene Integration

This document explains how to bring new 3D assets into SecretEngine, from raw GLB files to fully interactive entities in the Vulkan renderer.

---

## 1. Asset Cooking (GLB to .meshbin)

SecretEngine uses a custom binary format (`.meshbin`) for high-performance loading and rendering. It packages vertex positions, normals, and UVs into a single contiguous block.

### Step-by-Step Cooking:
1. Place your 3D model (in `.glb` format) in the `Assets/` directory.
2. Run the cooking script using Python:
   ```powershell
   python tools/cook_mesh.py Assets/MyModel.glb Assets/meshes/MyModel.meshbin
   ```
3. **Android Requirement**: After cooking, ensure the `.meshbin` is copied to the Android assets folder:
   ```
   android/app/src/main/assets/meshes/MyModel.meshbin
   ```

---

---

## 2. Adding to the Scene (Preferred Method)

The preferred way to add entities to the scene is via `scene.json`. This avoids hardcoding entity creation in C++.

### `scene.json` Structure:

```json
{
  "entities": [
    {
      "name": "PlayerStart",
      "transform": {
        "position": [0.0, 5.0, 10.0],
        "rotation": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0]
      }
    },
    {
      "name": "Character",
      "mesh": "meshes/cube.meshbin",
      "transform": {
        "position": [0.0, -1.0, 0.0],
        "rotation": [0.0, 0.0, 0.0],
        "scale": [15.0, 15.0, 15.0]
      },
      "color": [0.0, 1.0, 1.0, 1.0]
    }
  ]
}
```

### C++ Code Reference (Legacy/Internal):

You can still create entities manually if needed:

```cpp
// 1. Create a new Entity
auto world = m_core->GetWorld();
auto e = world->CreateEntity();

// 2. Define Transformation
auto transform = new SecretEngine::TransformComponent();
transform->position = {0.0f, -1.0f, 0.0f};
transform->scale = {15.0f, 15.0f, 15.0f};

// 3. Define Mesh Reference
auto mesh = new SecretEngine::MeshComponent();
strcpy(mesh->meshPath, "meshes/cube.meshbin");

// 4. Attach to World
world->AddComponent(e, SecretEngine::TransformComponent::TypeID, transform);
world->AddComponent(e, SecretEngine::MeshComponent::TypeID, mesh);
```

---

## 3. Reference for Renderer

### Hardware Initialization
Before rendering as an entity, the mesh data must be uploaded to the GPU via the 3D pipeline:
```cpp
m_pipeline3D->LoadMesh("meshes/MyModel.meshbin");
```

### Instancing Details
SecretEngine uses two forms of instancing:
1. **Pipeline3D (Legacy)**: Up to 100 instances per mesh. Good for characters.
2. **Mega Geometry (New)**: Up to **65,536 instances** in a single draw call. Good for massive environments (cities, foliage).
- **Performance**: Very high efficiency for repetitive objects (like trees or projectiles).
- **Control**: Each instance has its own unique `Transform` and `Color`, but shares the same Geometry.

---

## 4. Transformation Rules
- **Coordinate System**: Right-handed, Z-up coordinates (X=right, Y=forward, Z=up).
- **Scale**: A scale of `1.0` typically equals 1 meter. For large characters, `15.0` is a recommended starting point.
- **Rotation**: Applied in order: Y (Yaw), then X (Pitch), then Z (Roll).

---

## 6. Fast Asset Signaling (8-Byte Protocol)

To maintain ultra-low latency, SecretEngine uses the 8-byte **Fast Data Architecture (FDA)** to signal asset-related events between threads.

### Signaling Patterns:
1.  **Request Asset**: `AssetSignal` packet with Metadata = AssetID (Hash).
2.  **Asset Ready**: Fired by the Asset Loader to signal the Renderer that a mesh is uploaded.
3.  **Hot Reload**: Signal the engine to re-bind a shader or texture without stalling the frame.

**Requirement:** All new plugins must use `Fast::UltraPacket` with `PacketType::AssetSignal` for high-frequency asset events.
