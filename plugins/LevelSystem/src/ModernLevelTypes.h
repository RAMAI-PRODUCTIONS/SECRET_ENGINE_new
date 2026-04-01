// SecretEngine
// Module: ModernLevelTypes
// Responsibility: Modern level system types for v7_3 reference format
// Architecture: Load-Once-Spawn-Dereference

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace SecretEngine::Levels {

// ============================================================================
// Schema Version
// ============================================================================

struct SchemaVersion {
    uint32_t major = 1;
    uint32_t minor = 0;
    
    bool IsCompatible(const SchemaVersion& other) const {
        return major == other.major;
    }
    
    std::string ToString() const {
        return std::to_string(major) + "." + std::to_string(minor);
    }
};

// ============================================================================
// Transform Settings (Global for Level)
// ============================================================================

struct TransformSettings {
    enum class RotationFormat { Euler, Quaternion };
    enum class RotationUnit { Degrees, Radians };
    enum class RotationOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };
    
    RotationFormat rotationFormat = RotationFormat::Euler;
    RotationUnit rotationUnit = RotationUnit::Degrees;
    RotationOrder rotationOrder = RotationOrder::YXZ;
    
    static RotationFormat ParseRotationFormat(const std::string& str) {
        if (str == "quaternion") return RotationFormat::Quaternion;
        return RotationFormat::Euler;
    }
    
    static RotationUnit ParseRotationUnit(const std::string& str) {
        if (str == "radians") return RotationUnit::Radians;
        return RotationUnit::Degrees;
    }
    
    static RotationOrder ParseRotationOrder(const std::string& str) {
        if (str == "XYZ") return RotationOrder::XYZ;
        if (str == "XZY") return RotationOrder::XZY;
        if (str == "YXZ") return RotationOrder::YXZ;
        if (str == "YZX") return RotationOrder::YZX;
        if (str == "ZXY") return RotationOrder::ZXY;
        if (str == "ZYX") return RotationOrder::ZYX;
        return RotationOrder::YXZ; // Default
    }
};

// ============================================================================
// LOD Configuration
// ============================================================================

struct LODLevel {
    float distance = 0.0f;
    std::string meshPath;      // Temporary during load
    uint32_t meshHandle = 0;   // Runtime handle after load
};

struct LODConfig {
    bool enabled = true;
    std::vector<LODLevel> levels;
    
    // Get appropriate LOD level based on distance
    int GetLODIndex(float distance) const {
        if (!enabled || levels.empty()) return 0;
        
        for (size_t i = 0; i < levels.size(); ++i) {
            if (distance < levels[i].distance) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(levels.size() - 1);
    }
};

// ============================================================================
// Culling Configuration
// ============================================================================

struct CullingConfig {
    enum class Method { Sphere, AABB, OBB };
    
    bool enabled = true;
    Method method = Method::Sphere;
    float radius = 1.0f;
    float bounds[6] = {0}; // min xyz, max xyz for AABB/OBB
    
    static Method ParseMethod(const std::string& str) {
        if (str == "aabb") return Method::AABB;
        if (str == "obb") return Method::OBB;
        return Method::Sphere;
    }
};

// ============================================================================
// Instance Data (Temporary - Only During Spawn)
// ============================================================================

struct InstanceData {
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float scale[3] = {1, 1, 1};
    float materialColor[3] = {1, 1, 1};
    
    LODConfig lod;
    CullingConfig culling;
    
    // Metadata for spawning (temporary)
    std::string meshPath;
    std::string materialPath;
    nlohmann::json tags;
};

// ============================================================================
// Mesh Group (Temporary - Only During Load)
// ============================================================================

struct MeshGroup {
    std::string meshPath;
    std::string materialPath;
    std::vector<InstanceData> instances;
};

// ============================================================================
// Chunk Data (Temporary - Only During Load)
// ============================================================================

struct ChunkData {
    std::string chunkId;
    float boundsCenter[3] = {0, 0, 0};
    float boundsExtents[3] = {50, 50, 50};
    
    std::vector<MeshGroup> meshGroups;
    std::vector<nlohmann::json> entities; // Raw entity JSON
    
    // Calculate memory footprint
    size_t EstimateMemoryUsage() const {
        size_t total = sizeof(ChunkData);
        total += meshGroups.size() * sizeof(MeshGroup);
        for (const auto& group : meshGroups) {
            total += group.instances.size() * sizeof(InstanceData);
        }
        total += entities.size() * 512; // Estimate per entity JSON
        return total;
    }
};

// ============================================================================
// Level Reference Data (Loaded Once, Then Dereferenced)
// ============================================================================

struct LevelReferenceData {
    SchemaVersion schemaVersion;
    std::string levelName;
    TransformSettings transformSettings;
    
    // Metadata
    std::string author;
    std::string created;
    std::string engineVersion;
    
    // References (paths only)
    std::vector<std::string> entityPaths;
    std::vector<std::string> chunkPaths;
    std::string navigationPath;
    
    // Streaming config
    float chunkSize = 100.0f;
    int loadRadius = 2;
    int unloadRadius = 3;
    std::string streamingMethod = "distance";
    
    // Rendering defaults
    std::vector<float> defaultLODDistances;
    bool cullingEnabled = true;
    
    // Calculate memory footprint
    size_t EstimateMemoryUsage() const {
        size_t total = sizeof(LevelReferenceData);
        total += levelName.capacity();
        total += author.capacity();
        total += chunkPaths.size() * 64; // Estimate per path
        return total;
    }
};

// ============================================================================
// Runtime Chunk (After Instantiation)
// ============================================================================

struct RuntimeChunk {
    uint32_t chunkId = 0;
    float boundsCenter[3] = {0, 0, 0};
    float boundsExtents[3] = {50, 50, 50};
    
    // Entity IDs spawned in this chunk
    std::vector<uint32_t> entityIds;
    
    // State
    bool isLoaded = false;
    bool isVisible = false;
    float distanceFromPlayer = 0.0f;
    float timeLoaded = 0.0f;
    
    // Check if point is inside chunk bounds
    bool ContainsPoint(const float point[3]) const {
        for (int i = 0; i < 3; ++i) {
            float min = boundsCenter[i] - boundsExtents[i];
            float max = boundsCenter[i] + boundsExtents[i];
            if (point[i] < min || point[i] > max) {
                return false;
            }
        }
        return true;
    }
    
    // Calculate distance from point to chunk bounds
    float DistanceToPoint(const float point[3]) const {
        float dx = std::max(0.0f, std::abs(point[0] - boundsCenter[0]) - boundsExtents[0]);
        float dy = std::max(0.0f, std::abs(point[1] - boundsCenter[1]) - boundsExtents[1]);
        float dz = std::max(0.0f, std::abs(point[2] - boundsCenter[2]) - boundsExtents[2]);
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

// ============================================================================
// Runtime Level (After JSON Dereferenced)
// ============================================================================

struct RuntimeLevel {
    std::string name;
    SchemaVersion schemaVersion;
    TransformSettings transformSettings;
    
    // Runtime chunks
    std::vector<RuntimeChunk> chunks;
    
    // Streaming state
    float chunkSize = 100.0f;
    int loadRadius = 2;
    int unloadRadius = 3;
    
    // Stats
    uint32_t totalEntities = 0;
    uint32_t loadedChunks = 0;
    uint32_t visibleChunks = 0;
    
    // Get chunk by ID
    RuntimeChunk* GetChunk(uint32_t chunkId) {
        for (auto& chunk : chunks) {
            if (chunk.chunkId == chunkId) {
                return &chunk;
            }
        }
        return nullptr;
    }
    
    // Get chunk containing point
    RuntimeChunk* GetChunkAtPoint(const float point[3]) {
        for (auto& chunk : chunks) {
            if (chunk.ContainsPoint(point)) {
                return &chunk;
            }
        }
        return nullptr;
    }
    
    // Calculate memory footprint
    size_t EstimateMemoryUsage() const {
        size_t total = sizeof(RuntimeLevel);
        total += chunks.size() * sizeof(RuntimeChunk);
        for (const auto& chunk : chunks) {
            total += chunk.entityIds.size() * sizeof(uint32_t);
        }
        return total;
    }
};

} // namespace SecretEngine::Levels
