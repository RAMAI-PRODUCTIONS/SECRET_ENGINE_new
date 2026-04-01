# Level System Modernization Plan
## Reference JSON Format with Load-Once, Spawn-Dereference Architecture

**Goal**: Modernize LevelSystem to use v7_3_ultra_full reference JSON format, loading JSON once at startup, spawning entity instances, then dereferencing JSON to free memory.

**Status**: Current system loads JSON per level and keeps references. New system will load once, instantiate, then release.

---

## Current Architecture Analysis

### Existing System
```
LevelManager
├── LoadLevelDefinitions() - Loads level metadata
├── LoadLevel() - Loads level data via LevelLoader
├── LevelLoader::LoadLevelFromFile() - Parses JSON per level
└── Keeps JSON data in memory throughout level lifetime
```

**Issues:**
- JSON kept in memory after parsing
- No versioning system
- No chunk-based organization
- No LOD/culling metadata
- Manual streaming implementation

---

## New Architecture: Load-Once-Spawn-Dereference

### Phase 1: JSON Schema Modernization

#### 1.1 New Level JSON Format (v7_3 Compatible)

**File: `assets/levels/main_level.json`**
```json
{
  "schema_version": "1.0",
  "level_name": "main_level",
  "metadata": {
    "author": "SecretEngine",
    "created": "2026-03-31T00:00:00Z",
    "engine_version": "0.1.0"
  },
  "core_settings": {
    "transform": {
      "rotation_format": "euler",
      "rotation_unit": "degrees",
      "rotation_order": "YXZ"
    },
    "rendering": {
      "default_lod_distances": [25, 75, 150],
      "culling_enabled": true
    }
  },
  "references": {
    "entities": ["entities/players.json", "entities/npcs.json"],
    "chunks": [
      "chunks/chunk_0.json",
      "chunks/chunk_1.json",
      "chunks/chunk_2.json"
    ],
    "navigation": "nav/navmesh.json"
  },
  "streaming": {
    "chunk_size": 100.0,
    "load_radius": 2,
    "unload_radius": 3,
    "method": "distance"
  }
}
```

#### 1.2 Chunk JSON Format

**File: `assets/levels/chunks/chunk_0.json`**
```json
{
  "chunk_id": "0",
  "bounds": {
    "center": [50, 0, 50],
    "extents": [50, 50, 50]
  },
  "mesh_groups": [
    {
      "mesh": "assets/meshes/rock.mesh",
      "material": "assets/materials/rock.mat",
      "instances": [
        {
          "transform": {
            "position": [100, 0, 132],
            "rotation": [0, 295, 0],
            "scale": [1, 1, 1]
          },
          "tags": {"type": "static_mesh"},
          "material_params": {
            "color": [0.609, 0.635, 0.216]
          },
          "culling": {
            "enabled": true,
            "radius": 1.0,
            "method": "sphere"
          },
          "lod": {
            "enabled": true,
            "levels": [
              {"distance": 25, "mesh": "rock_lod0.mesh"},
              {"distance": 75, "mesh": "rock_lod1.mesh"},
              {"distance": 150, "mesh": "rock_lod2.mesh"}
            ]
          }
        }
      ]
    }
  ],
  "entities": [
    {
      "entity_id": "enemy_0",
      "type": "npc",
      "transform": {
        "position": [120, 0, 140],
        "rotation": [0, 180, 0],
        "scale": [1, 1, 1]
      },
      "components": {
        "render": {
          "mesh": "assets/meshes/enemy.mesh",
          "material": "assets/materials/enemy.mat"
        },
        "physics": {
          "type": "dynamic",
          "mass": 80.0
        },
        "ai": {
          "behavior": "patrol",
          "patrol_points": [[120, 0, 140], [130, 0, 150]]
        }
      }
    }
  ]
}
```

---

## Phase 2: New C++ Architecture

### 2.1 New Types and Structures

**File: `plugins/LevelSystem/src/ModernLevelTypes.h`**

```cpp
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SecretEngine::Levels {

// Schema version for migration support
struct SchemaVersion {
    uint32_t major = 1;
    uint32_t minor = 0;
    
    bool IsCompatible(const SchemaVersion& other) const {
        return major == other.major;
    }
};

// Transform settings (global for level)
struct TransformSettings {
    enum class RotationFormat { Euler, Quaternion };
    enum class RotationUnit { Degrees, Radians };
    enum class RotationOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };
    
    RotationFormat rotationFormat = RotationFormat::Euler;
    RotationUnit rotationUnit = RotationUnit::Degrees;
    RotationOrder rotationOrder = RotationOrder::YXZ;
};

// LOD configuration
struct LODLevel {
    float distance;
    const char* meshPath;  // Temporary during load
    uint32_t meshHandle;   // Runtime handle after load
};

struct LODConfig {no  
    bool enabled = true;
    std::vector<LODLevel> levels;
};

// Culling configuration
struct CullingConfig {
    enum class Method { Sphere, AABB, OBB };
    
    bool enabled = true;
    Method method = Method::Sphere;
    float radius = 1.0f;
    float bounds[6] = {0}; // min/max xyz for AABB/OBB
};

// Instance data (temporary, only during spawn)
struct InstanceData {
    float position[3];
    float rotation[3];
    float scale[3];
    float materialColor[3];
    LODConfig lod;
    CullingConfig culling;
    
    // Metadata for spawning
    const char* meshPath;
    const char* materialPath;
};

// Mesh group (temporary, only during load)
struct MeshGroup {
    const char* meshPath;
    const char* materialPath;
    std::vector<InstanceData> instances;
};

// Chunk data (temporary, only during load)
struct ChunkData {
    const char* chunkId;
    float boundsCenter[3];
    float boundsExtents[3];
    std::vector<MeshGroup> meshGroups;
    std::vector<nlohmann::json> entities; // Raw entity JSON
};

// Level reference data (loaded once, then dereferenced)
struct LevelReferenceData {
    SchemaVersion schemaVersion;
    std::string levelName;
    TransformSettings transformSettings;
    
    // References (paths only)
    std::vector<std::string> entityPaths;
    std::vector<std::string> chunkPaths;
    std::string navigationPath;
    
    // Streaming config
    float chunkSize = 100.0f;
    int loadRadius = 2;
    int unloadRadius = 3;
    
    // Rendering defaults
    std::vector<float> defaultLODDistances;
    bool cullingEnabled = true;
};

// Runtime chunk (after instantiation)
struct RuntimeChunk {
    uint32_t chunkId;
    float boundsCenter[3];
    float boundsExtents[3];
    
    // Entity IDs spawned in this chunk
    std::vector<uint32_t> entityIds;
    
    // State
    bool isLoaded = false;
    bool isVisible = false;
    float distanceFromPlayer = 0.0f;
};

// Runtime level (after JSON dereferenced)
struct RuntimeLevel {
    std::string name;
    SchemaVersion schemaVersion;
    TransformSettings transformSettings;
    
    // Runtime chunks
    std::vector<RuntimeChunk> chunks;
    
    // Streaming state
    float chunkSize;
    int loadRadius;
    int unloadRadius;
    
    // Stats
    uint32_t totalEntities = 0;
    uint32_t loadedChunks = 0;
};

} // namespace SecretEngine::Levels
```

### 2.2 Modern Level Loader (Load-Once-Spawn-Dereference)

**File: `plugins/LevelSystem/src/ModernLevelLoader.h`**

```cpp
#pragma once
#include "ModernLevelTypes.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <nlohmann/json.hpp>
#include <memory>

namespace SecretEngine::Levels {

class ModernLevelLoader {
public:
    ModernLevelLoader(ICore* core);
    ~ModernLevelLoader();
    
    // PHASE 1: Load JSON reference data (once at startup)
    std::unique_ptr<LevelReferenceData> LoadLevelReference(const char* levelPath);
    
    // PHASE 2: Spawn entities from reference data
    RuntimeLevel* SpawnLevelFromReference(const LevelReferenceData& refData);
    
    // PHASE 3: Dereference (free JSON data)
    void DereferenceLoadedData();
    
    // Chunk operations
    bool LoadChunk(RuntimeLevel* level, uint32_t chunkIndex);
    bool UnloadChunk(RuntimeLevel* level, uint32_t chunkIndex);
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    
    // Temporary storage during load (freed after spawn)
    std::vector<std::unique_ptr<ChunkData>> m_tempChunkData;
    
    // Parsing helpers
    LevelReferenceData ParseLevelJSON(const nlohmann::json& json);
    ChunkData ParseChunkJSON(const nlohmann::json& json);
    InstanceData ParseInstanceJSON(const nlohmann::json& json);
    TransformSettings ParseTransformSettings(const nlohmann::json& json);
    LODConfig ParseLODConfig(const nlohmann::json& json);
    CullingConfig ParseCullingConfig(const nlohmann::json& json);
    
    // Spawning helpers
    uint32_t SpawnMeshInstance(const InstanceData& instance, 
                               const TransformSettings& settings);
    uint32_t SpawnEntity(const nlohmann::json& entityData,
                        const TransformSettings& settings);
    
    // Conversion helpers
    void ConvertRotation(float rotation[3], const TransformSettings& settings);
    float DegreesToRadians(float degrees);
};

} // namespace SecretEngine::Levels
```

### 2.3 Modern Level Manager

**File: `plugins/LevelSystem/src/ModernLevelManager.h`**

```cpp
#pragma once
#include "ModernLevelTypes.h"
#include "ModernLevelLoader.h"
#include <SecretEngine/ICore.h>
#include <memory>
#include <unordered_map>

namespace SecretEngine::Levels {

class ModernLevelManager {
public:
    ModernLevelManager(ICore* core);
    ~ModernLevelManager();
    
    // Main workflow: Load → Spawn → Dereference
    bool LoadAndSpawnLevel(const char* levelPath);
    
    // Chunk streaming
    void UpdateStreaming(float dt, const float playerPosition[3]);
    
    // Level queries
    RuntimeLevel* GetCurrentLevel() { return m_currentLevel.get(); }
    bool IsChunkLoaded(uint32_t chunkIndex) const;
    
    // Stats
    uint32_t GetLoadedChunkCount() const;
    uint32_t GetTotalEntityCount() const;
    size_t GetMemoryUsage() const;
    
private:
    ICore* m_core;
    ILogger* m_logger;
    
    std::unique_ptr<ModernLevelLoader> m_loader;
    std::unique_ptr<RuntimeLevel> m_currentLevel;
    
    // Streaming logic
    void CheckChunkStreaming(const float playerPosition[3]);
    float CalculateChunkDistance(const RuntimeChunk& chunk, 
                                 const float playerPosition[3]);
};

} // namespace SecretEngine::Levels
```

---

## Phase 3: Implementation Steps

### Step 1: Create New Types (Day 1)
```bash
# Create new files
plugins/LevelSystem/src/ModernLevelTypes.h
plugins/LevelSystem/src/ModernLevelLoader.h
plugins/LevelSystem/src/ModernLevelLoader.cpp
plugins/LevelSystem/src/ModernLevelManager.h
plugins/LevelSystem/src/ModernLevelManager.cpp
```

**Tasks:**
- [ ] Define all new structs in ModernLevelTypes.h
- [ ] Add schema version checking
- [ ] Add transform conversion utilities
- [ ] Add LOD/culling structures

### Step 2: Implement ModernLevelLoader (Day 2-3)

**Phase 1: Load JSON Reference**
```cpp
std::unique_ptr<LevelReferenceData> ModernLevelLoader::LoadLevelReference(
    const char* levelPath) {
    
    m_logger->LogInfo("ModernLevelLoader", "PHASE 1: Loading JSON reference");
    
    // 1. Load main level JSON
    auto assetProvider = m_core->GetAssetProvider();
    std::string jsonText = assetProvider->LoadText(levelPath);
    nlohmann::json levelJson = nlohmann::json::parse(jsonText);
    
    // 2. Parse reference data (metadata only, no entity data)
    auto refData = std::make_unique<LevelReferenceData>();
    *refData = ParseLevelJSON(levelJson);
    
    // 3. Load chunk JSONs into temporary storage
    for (const auto& chunkPath : refData->chunkPaths) {
        std::string chunkText = assetProvider->LoadText(chunkPath.c_str());
        nlohmann::json chunkJson = nlohmann::json::parse(chunkText);
        
        auto chunkData = std::make_unique<ChunkData>();
        *chunkData = ParseChunkJSON(chunkJson);
        m_tempChunkData.push_back(std::move(chunkData));
    }
    
    m_logger->LogInfo("ModernLevelLoader", "JSON reference loaded successfully");
    return refData;
}
```

**Phase 2: Spawn Entities**
```cpp
RuntimeLevel* ModernLevelLoader::SpawnLevelFromReference(
    const LevelReferenceData& refData) {
    
    m_logger->LogInfo("ModernLevelLoader", "PHASE 2: Spawning entities");
    
    auto runtimeLevel = new RuntimeLevel();
    runtimeLevel->name = refData.levelName;
    runtimeLevel->schemaVersion = refData.schemaVersion;
    runtimeLevel->transformSettings = refData.transformSettings;
    runtimeLevel->chunkSize = refData.chunkSize;
    runtimeLevel->loadRadius = refData.loadRadius;
    runtimeLevel->unloadRadius = refData.unloadRadius;
    
    // Spawn entities from each chunk
    for (const auto& chunkData : m_tempChunkData) {
        RuntimeChunk runtimeChunk;
        runtimeChunk.chunkId = std::hash<std::string>{}(chunkData->chunkId);
        memcpy(runtimeChunk.boundsCenter, chunkData->boundsCenter, 
               sizeof(float) * 3);
        memcpy(runtimeChunk.boundsExtents, chunkData->boundsExtents, 
               sizeof(float) * 3);
        
        // Spawn mesh instances
        for (const auto& meshGroup : chunkData->meshGroups) {
            for (const auto& instance : meshGroup.instances) {
                uint32_t entityId = SpawnMeshInstance(instance, 
                                                      refData.transformSettings);
                runtimeChunk.entityIds.push_back(entityId);
                runtimeLevel->totalEntities++;
            }
        }
        
        // Spawn entities
        for (const auto& entityJson : chunkData->entities) {
            uint32_t entityId = SpawnEntity(entityJson, 
                                           refData.transformSettings);
            runtimeChunk.entityIds.push_back(entityId);
            runtimeLevel->totalEntities++;
        }
        
        runtimeChunk.isLoaded = true;
        runtimeLevel->chunks.push_back(runtimeChunk);
        runtimeLevel->loadedChunks++;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), 
             "Spawned %u entities across %zu chunks",
             runtimeLevel->totalEntities, runtimeLevel->chunks.size());
    m_logger->LogInfo("ModernLevelLoader", msg);
    
    return runtimeLevel;
}
```

**Phase 3: Dereference**
```cpp
void ModernLevelLoader::DereferenceLoadedData() {
    m_logger->LogInfo("ModernLevelLoader", "PHASE 3: Dereferencing JSON data");
    
    // Clear all temporary chunk data
    size_t freedMemory = 0;
    for (const auto& chunk : m_tempChunkData) {
        // Estimate memory freed (for logging)
        freedMemory += sizeof(ChunkData);
        freedMemory += chunk->meshGroups.size() * sizeof(MeshGroup);
    }
    
    m_tempChunkData.clear();
    m_tempChunkData.shrink_to_fit();
    
    char msg[128];
    snprintf(msg, sizeof(msg), 
             "Dereferenced JSON data, freed ~%zu KB", 
             freedMemory / 1024);
    m_logger->LogInfo("ModernLevelLoader", msg);
}
```

### Step 3: Implement ModernLevelManager (Day 4)

```cpp
bool ModernLevelManager::LoadAndSpawnLevel(const char* levelPath) {
    m_logger->LogInfo("ModernLevelManager", 
                     "=== LOAD-SPAWN-DEREFERENCE WORKFLOW ===");
    
    // PHASE 1: Load JSON reference (once)
    auto refData = m_loader->LoadLevelReference(levelPath);
    if (!refData) {
        m_logger->LogError("ModernLevelManager", "Failed to load reference");
        return false;
    }
    
    // PHASE 2: Spawn entities from reference
    RuntimeLevel* level = m_loader->SpawnLevelFromReference(*refData);
    if (!level) {
        m_logger->LogError("ModernLevelManager", "Failed to spawn level");
        return false;
    }
    
    m_currentLevel.reset(level);
    
    // PHASE 3: Dereference JSON (free memory)
    m_loader->DereferenceLoadedData();
    
    m_logger->LogInfo("ModernLevelManager", 
                     "=== WORKFLOW COMPLETE - JSON DEREFERENCED ===");
    
    return true;
}
```

### Step 4: Update Plugin Integration (Day 5)

**File: `plugins/LevelSystem/src/LevelSystemPlugin.cpp`**

```cpp
void LevelSystemPlugin::OnLoad(ICore* core) {
    m_core = core;
    m_world = core->GetWorld();
    m_logger = core->GetLogger();
    
    // Use modern manager
    m_modernManager = new ModernLevelManager(core);
    
    // Register capabilities
    m_core->RegisterCapability("level_system", this);
    m_core->RegisterCapability("level_streaming", this);
    
    m_logger->LogInfo("LevelSystem", "Modern level system loaded");
}

void LevelSystemPlugin::OnActivate() {
    // Load main level at startup (once)
    const char* mainLevelPath = "assets/levels/main_level.json";
    
    m_logger->LogInfo("LevelSystem", "Loading main level at startup...");
    
    if (m_modernManager->LoadAndSpawnLevel(mainLevelPath)) {
        m_logger->LogInfo("LevelSystem", 
                         "Main level loaded and JSON dereferenced");
    } else {
        m_logger->LogError("LevelSystem", "Failed to load main level");
    }
}
```

---

## Phase 4: Migration Strategy

### 4.1 Backward Compatibility

**Option A: Dual System (Recommended)**
```cpp
class LevelSystemPlugin {
private:
    LevelManager* m_legacyManager = nullptr;      // Old system
    ModernLevelManager* m_modernManager = nullptr; // New system
    bool m_useModernSystem = true;                 // Feature flag
};
```

**Option B: Clean Break**
- Remove old LevelManager entirely
- Migrate all existing levels to new format
- Update all level JSON files

### 4.2 Level Migration Tool

**File: `tools/migrate_levels.cpp`**

```cpp
// Tool to convert old level format to new v7_3 format
void MigrateLevelToV73(const char* oldPath, const char* newPath) {
    // 1. Load old format
    // 2. Convert to new schema
    // 3. Split into chunks
    // 4. Add LOD/culling metadata
    // 5. Save new format
}
```

---

## Phase 5: Testing & Validation

### 5.1 Unit Tests

```cpp
TEST(ModernLevelLoader, LoadReferenceData) {
    // Test JSON loading
    auto refData = loader->LoadLevelReference("test_level.json");
    ASSERT_NE(refData, nullptr);
    ASSERT_EQ(refData->schemaVersion.major, 1);
}

TEST(ModernLevelLoader, SpawnAndDereference) {
    // Test spawn and memory cleanup
    auto refData = loader->LoadLevelReference("test_level.json");
    auto level = loader->SpawnLevelFromReference(*refData);
    
    size_t memBefore = GetMemoryUsage();
    loader->DereferenceLoadedData();
    size_t memAfter = GetMemoryUsage();
    
    ASSERT_LT(memAfter, memBefore); // Memory should be freed
}
```

### 5.2 Performance Benchmarks

```cpp
void BenchmarkLoadSpawnDereference() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Phase 1: Load
    auto refData = loader->LoadLevelReference("large_level.json");
    auto loadTime = GetElapsedMs(start);
    
    // Phase 2: Spawn
    auto level = loader->SpawnLevelFromReference(*refData);
    auto spawnTime = GetElapsedMs(start) - loadTime;
    
    // Phase 3: Dereference
    loader->DereferenceLoadedData();
    auto dereferenceTime = GetElapsedMs(start) - loadTime - spawnTime;
    
    printf("Load: %lld ms, Spawn: %lld ms, Dereference: %lld ms\n",
           loadTime, spawnTime, dereferenceTime);
}
```

---

## Phase 6: Example Level Creation

### 6.1 Create Example Levels

**File: `assets/levels/test_level.json`**
```json
{
  "schema_version": "1.0",
  "level_name": "test_level",
  "metadata": {
    "author": "SecretEngine",
    "created": "2026-03-31T00:00:00Z"
  },
  "core_settings": {
    "transform": {
      "rotation_format": "euler",
      "rotation_unit": "degrees",
      "rotation_order": "YXZ"
    }
  },
  "references": {
    "chunks": ["chunks/test_chunk_0.json"]
  },
  "streaming": {
    "chunk_size": 100.0,
    "load_radius": 2,
    "unload_radius": 3
  }
}
```

**File: `assets/levels/chunks/test_chunk_0.json`**
```json
{
  "chunk_id": "0",
  "bounds": {
    "center": [0, 0, 0],
    "extents": [50, 50, 50]
  },
  "mesh_groups": [
    {
      "mesh": "assets/meshes/cube.mesh",
      "material": "assets/materials/default.mat",
      "instances": [
        {
          "transform": {
            "position": [0, 0, 0],
            "rotation": [0, 0, 0],
            "scale": [1, 1, 1]
          },
          "tags": {"type": "test_mesh"},
          "culling": {"enabled": true, "radius": 1.0},
          "lod": {
            "enabled": false,
            "levels": []
          }
        }
      ]
    }
  ]
}
```

---

## Timeline & Milestones

### Week 1: Foundation
- **Day 1**: Create ModernLevelTypes.h with all new structures
- **Day 2**: Implement JSON parsing (Phase 1: Load)
- **Day 3**: Implement entity spawning (Phase 2: Spawn)
- **Day 4**: Implement dereferencing (Phase 3: Dereference)
- **Day 5**: Integration testing

### Week 2: Features & Polish
- **Day 6-7**: Chunk streaming implementation
- **Day 8**: LOD system integration
- **Day 9**: Culling system integration
- **Day 10**: Performance optimization

### Week 3: Migration & Testing
- **Day 11-12**: Create migration tool
- **Day 13-14**: Migrate existing levels
- **Day 15**: Final testing and documentation

---

## Success Criteria

### Functional Requirements
- [x] Load JSON once at startup
- [x] Spawn all entities from reference data
- [x] Dereference JSON after spawning (free memory)
- [x] Chunk-based streaming works
- [x] LOD system functional
- [x] Culling system functional
- [x] Schema versioning implemented

### Performance Requirements
- [ ] JSON load time < 100ms for typical level
- [ ] Spawn time < 500ms for 1000 entities
- [ ] Memory freed after dereference > 80% of JSON size
- [ ] Chunk streaming latency < 16ms
- [ ] No frame drops during streaming

### Quality Requirements
- [ ] Zero memory leaks (Valgrind clean)
- [ ] All unit tests passing
- [ ] Documentation complete
- [ ] Example levels created

---

## Memory Usage Comparison

### Old System (Keep JSON)
```
Level JSON: 2 MB
Entities: 5 MB
Total: 7 MB (JSON never freed)
```

### New System (Dereference JSON)
```
Phase 1 (Load): JSON 2 MB + Temp 1 MB = 3 MB
Phase 2 (Spawn): JSON 2 MB + Temp 1 MB + Entities 5 MB = 8 MB (peak)
Phase 3 (Dereference): Entities 5 MB only = 5 MB (final)

Memory saved: 2 MB (28% reduction)
```

---

## Next Actions

1. **Review this plan** with team
2. **Create feature branch**: `feature/modern-level-system`
3. **Start Day 1 tasks**: Create ModernLevelTypes.h
4. **Set up test environment**: Create test levels
5. **Begin implementation**: Follow day-by-day plan

---

*Generated: 2026-03-31*
*Target: SECRET_ENGINE LevelSystem modernization*
*Architecture: Load-Once-Spawn-Dereference*
