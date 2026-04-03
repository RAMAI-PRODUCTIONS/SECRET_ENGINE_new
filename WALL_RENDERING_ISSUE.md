# Wall Rendering Issue - Root Cause Analysis

## Problem Statement
- 4009 instances are loaded into the flywheel
- GPU is processing 4009 instances (culling working)
- 24M triangles reported
- **BUT: No walls visible on screen**

## Root Cause: Mesh Not Loaded

### What's Happening
1. ✅ Chunk file loaded: `levelone_chunk.json`
2. ✅ 4009 entities created from chunk
3. ✅ 4009 instances added to MegaGeometryRenderer
4. ✅ GPU culling processing 4009 instances
5. ❌ **wall.meshbin NEVER LOADED**
6. ❌ Instances have no geometry to render

### Evidence from Logcat

#### Chunk Loaded
```
LevelLoader: [INFO] Loading chunk from: Assets/levels/chunks/levelone_chunk.json
LevelLoader: [INFO] Loaded 4009 entities from v7.3 level
```

#### Instances in Flywheel
```
MegaGeometryRenderer: [INFO] GPU Culling: 4009 instances, 16 workgroups
Profiler: [INFO] [RENDER] TRI:24254k INST:4009 DRAW:2
```

#### Only Character.meshbin Loaded
```
MegaGeometryRenderer: [INFO] LoadMesh: Loaded meshes/Character.meshbin - 6188 vertices, 18150 indices
VulkanRenderer: [INFO] Character mesh loaded into MegaGeometry slot 0
```

#### wall.meshbin NOT Found
```
(No log messages about wall.meshbin)
```

## The Issue

### Chunk File Says:
```json
{
  "mesh_groups": [
    {
      "mesh": "wall.meshbin",  // ← This mesh should be loaded
      "material": "default.mat",
      "instances": [ ... 4009 instances ... ]
    }
  ]
}
```

### What Actually Happens:
1. Chunk parser reads "wall.meshbin"
2. Creates 4009 entities
3. Adds 4009 instances to flywheel
4. **BUT: Never calls LoadMesh("wall.meshbin")**
5. Instances reference mesh slot that has no geometry
6. GPU tries to render but there's nothing to draw

## Why You See 4009 Instances But No Walls

The instances exist in memory and are being processed:
- Instance data: ✅ (positions, transforms, colors)
- GPU culling: ✅ (processing 4009 instances)
- Mesh geometry: ❌ (wall.meshbin not loaded)
- Result: **Invisible instances**

## Solution Required

The chunk loading system needs to:
1. Parse the chunk file
2. **Load the mesh file** (wall.meshbin)
3. Create instances
4. Link instances to the loaded mesh

### Current Flow (Broken):
```
Load Chunk → Parse JSON → Create Entities → Add Instances
                                              ↓
                                         (No mesh!)
```

### Required Flow (Fixed):
```
Load Chunk → Parse JSON → Load Mesh (wall.meshbin) → Create Entities → Add Instances
                              ↓                                           ↓
                         Mesh Slot 0                              Link to Slot 0
```

## Quick Fix

The LevelLoader needs to load meshes when processing chunks:

```cpp
// In LevelLoader when processing chunk
for (const auto& meshGroup : chunk["mesh_groups"]) {
    std::string meshPath = meshGroup["mesh"];
    
    // MISSING: Load the mesh!
    uint32_t meshSlot = LoadMeshIntoRenderer(meshPath);
    
    // Then create instances using that mesh slot
    for (const auto& instance : meshGroup["instances"]) {
        AddInstanceToRenderer(meshSlot, instance);
    }
}
```

## Current State

### Instance Cleanup: ✅ WORKING
- Instances are properly cleared on level change
- Memory management working correctly
- Cleanup system verified

### Mesh Loading: ❌ NOT WORKING
- Chunk meshes not being loaded
- Only Character.meshbin loads (from scene.json)
- 4009 instances have no geometry

## Next Steps

1. Find where chunks are processed
2. Add mesh loading when processing mesh_groups
3. Ensure wall.meshbin is loaded before creating instances
4. Link instances to correct mesh slot

## Files to Check

- `plugins/LevelSystem/src/LevelLoader.cpp` - Chunk processing
- `plugins/LevelSystem/src/LevelManager.cpp` - Level loading
- Look for where mesh_groups are parsed
- Add LoadMesh() call for each unique mesh in chunk

## Expected Result After Fix

```
LevelLoader: [INFO] Loading chunk from: Assets/levels/chunks/levelone_chunk.json
MegaGeometryRenderer: [INFO] LoadMesh: Loaded meshes/wall.meshbin - X vertices, Y indices  ← NEW
MegaGeometryRenderer: [INFO] LoadMesh: Mesh slot 1 initialized with indexCount=Y  ← NEW
LevelLoader: [INFO] Loaded 4009 entities from v7.3 level
MegaGeometryRenderer: [INFO] GPU Culling: 4009 instances, 16 workgroups
```

Then walls will be visible!
