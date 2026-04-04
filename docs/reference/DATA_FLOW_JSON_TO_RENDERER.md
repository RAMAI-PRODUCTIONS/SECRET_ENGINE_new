# Data Flow: JSON to MegaGeometry Renderer

## Problem
JSON references walls, but characters are being rendered. This means somewhere a variable is holding the wrong data between JSON loading and rendering.

## Complete Data Flow Chain

### 1. JSON File Structure
```json
{
  "chunks": [
    {
      "mesh_groups": [
        {
          "mesh": "meshes/Wall.meshbin",
          "material": "materials/stone.mat",
          "instances": [
            {
              "transform": { "position": [x, y, z], "rotation": [...], "scale": [...] },
              "material_params": { "color": [r, g, b, a] },
              "tags": { "type": "wall", ... }
            }
          ]
        }
      ]
    }
  ]
}
```

### 2. Level Loading (V73LevelManagerImpl.cpp)

**Function: `LoadLevel()`**
- Loads JSON text via `AssetProvider`
- Calls `LoadLevelFromJSON(jsonText)`

**Function: `LoadLevelFromJSON()`**
- Parses JSON using nlohmann::json
- Calls `ParseLevelJSON(json)`
- Stores result in `m_currentLevel` (unique_ptr<Level>)

**Function: `ParseLevelJSON()`**
- Iterates through `json["chunks"]`
- For each chunk, calls `ParseChunkJSON(chunkJson)`
- Stores chunks in `level.chunks` vector

**Function: `ParseChunkJSON()`**
- **CRITICAL SECTION**: Parses mesh_groups
```cpp
if (json.contains("mesh_groups") && json["mesh_groups"].is_array()) {
    for (const auto& meshGroup : json["mesh_groups"]) {
        std::string mesh = meshGroup.value("mesh", "");  // ← WALL MESH PATH
        std::string material = meshGroup.value("material", "");
        
        if (meshGroup.contains("instances") && meshGroup["instances"].is_array()) {
            for (const auto& instance : meshGroup["instances"]) {
                MeshInstance meshInstance;
                meshInstance.id = static_cast<uint32_t>(chunk.mesh_instances.size());
                
                // Parse transform
                if (instance.contains("transform")) {
                    meshInstance.transform = ParseTransformJSON(instance["transform"]);
                }
                
                // Parse color
                if (instance.contains("material_params")) {
                    const auto& params = instance["material_params"];
                    if (params.contains("color") && params["color"].is_array()) {
                        auto color = params["color"];
                        meshInstance.color = glm::vec4(
                            color[0].get<float>(),
                            color[1].get<float>(),
                            color[2].get<float>(),
                            1.0f
                        );
                    }
                }
                
                chunk.mesh_instances.push_back(meshInstance);  // ← STORED HERE
            }
        }
    }
}
```

**Data Structure: `MeshInstance`** (in V73LevelManager.h)
```cpp
struct MeshInstance {
    uint32_t id;
    Transform transform;  // position, rotation, scale
    glm::vec4 color;
    // ... other fields
    // ⚠️ NOTE: NO MESH PATH STORED HERE!
};
```

### 3. Modern Level Loader (ModernLevelLoader.cpp)

**Function: `SpawnMeshInstance()`**
- Creates entity: `Entity entity = m_world->CreateEntity()`
- Adds TransformComponent with position/rotation/scale
- **CRITICAL**: Adds MeshComponent
```cpp
MeshComponent mesh;
strncpy(mesh.meshPath, instance.meshPath.c_str(),  // ← WHERE IS instance.meshPath?
        sizeof(mesh.meshPath) - 1);
mesh.color[0] = instance.materialColor[0];
mesh.color[1] = instance.materialColor[1];
mesh.color[2] = instance.materialColor[2];
mesh.color[3] = 1.0f;

m_world->AddComponent(entity, MeshComponent::TypeID, &mesh);
```

**Data Structure: `InstanceData`** (in ModernLevelTypes.h)
```cpp
struct InstanceData {
    std::string meshPath;      // ← MESH PATH SHOULD BE HERE
    std::string materialPath;
    float position[3];
    float rotation[3];
    float scale[3];
    float materialColor[4];
    LODConfig lod;
    CullingConfig culling;
};
```

### 4. Mesh Rendering System (MeshRenderingSystem.cpp)

**Function: `ProcessNewEntities()`**
- Queries all entities from world
- For each entity with MeshComponent + TransformComponent:
```cpp
auto* meshComp = static_cast<MeshComponent*>(m_world->GetComponent(entity, MeshComponent::TypeID));
auto* transformComp = static_cast<TransformComponent*>(m_world->GetComponent(entity, TransformComponent::TypeID));

// Load mesh if needed (assume Character.meshbin for now, slot 0)
uint32_t meshSlot = 0;  // ← HARDCODED TO SLOT 0!

// Add instance to renderer
uint32_t instanceID = m_megaGeometry->AddInstance(
    meshSlot,  // ← ALWAYS SLOT 0 (Character.meshbin)
    transformComp->position[0],
    transformComp->position[1],
    transformComp->position[2],
    textureID
);

// Set color from mesh component
m_megaGeometry->UpdateInstanceColor(instanceID,
    meshComp->color[0],  // ← COLOR FROM JSON
    meshComp->color[1],
    meshComp->color[2],
    meshComp->color[3]
);
```

### 5. MegaGeometry Renderer

**Mesh Slot System:**
- Slot 0: Character.meshbin (loaded in RendererPlugin.cpp:191)
- Slot 1-15: Available for other meshes
- **Problem**: All instances use slot 0 regardless of JSON mesh path

## THE BUG: Missing Mesh Path Propagation

### Issue 1: MeshInstance doesn't store mesh path
In `V73LevelManagerImpl.cpp::ParseChunkJSON()`:
```cpp
std::string mesh = meshGroup.value("mesh", "");  // ← MESH PATH READ
// ... but never stored in meshInstance!
chunk.mesh_instances.push_back(meshInstance);
```

The `MeshInstance` struct has no `meshPath` field!

### Issue 2: ModernLevelLoader expects meshPath
In `ModernLevelLoader.cpp::SpawnMeshInstance()`:
```cpp
strncpy(mesh.meshPath, instance.meshPath.c_str(), ...);  // ← Expects meshPath
```

But `InstanceData` has `meshPath`, while `MeshInstance` (from V73) doesn't!

### Issue 3: MeshRenderingSystem ignores mesh path
In `MeshRenderingSystem.cpp::ProcessNewEntities()`:
```cpp
// Load mesh if needed (assume Character.meshbin for now, slot 0)
uint32_t meshSlot = 0;  // ← HARDCODED!
```

Even if `meshComp->meshPath` contains "Wall.meshbin", it's ignored!

## Solution Required

### Fix 1: Add meshPath to MeshInstance
```cpp
struct MeshInstance {
    uint32_t id;
    std::string meshPath;  // ← ADD THIS
    Transform transform;
    glm::vec4 color;
    // ...
};
```

### Fix 2: Store mesh path during parsing
```cpp
for (const auto& meshGroup : json["mesh_groups"]) {
    std::string mesh = meshGroup.value("mesh", "");
    
    for (const auto& instance : meshGroup["instances"]) {
        MeshInstance meshInstance;
        meshInstance.meshPath = mesh;  // ← STORE IT
        // ... rest of parsing
    }
}
```

### Fix 3: Use mesh path in MeshRenderingSystem
```cpp
// Map mesh path to slot
uint32_t meshSlot = GetOrLoadMeshSlot(meshComp->meshPath);

uint32_t instanceID = m_megaGeometry->AddInstance(
    meshSlot,  // ← USE CORRECT SLOT
    transformComp->position[0],
    // ...
);
```

### Fix 4: Implement mesh slot management
```cpp
std::unordered_map<std::string, uint32_t> m_meshPathToSlot;

uint32_t GetOrLoadMeshSlot(const char* meshPath) {
    auto it = m_meshPathToSlot.find(meshPath);
    if (it != m_meshPathToSlot.end()) {
        return it->second;
    }
    
    // Find free slot
    uint32_t slot = m_nextFreeSlot++;
    if (m_megaGeometry->LoadMesh(meshPath, slot)) {
        m_meshPathToSlot[meshPath] = slot;
        return slot;
    }
    
    return 0; // Fallback to slot 0
}
```

## Summary

The wall data IS being loaded from JSON and stored in the level structure, but:
1. The mesh path ("meshes/Wall.meshbin") is NOT stored in the MeshInstance
2. When entities are spawned, they get the color but not the mesh path
3. MeshRenderingSystem hardcodes slot 0 (Character.meshbin) for all entities
4. Result: Wall-colored characters instead of walls

The fix requires propagating the mesh path through the entire chain and implementing proper mesh slot management in the rendering system.
