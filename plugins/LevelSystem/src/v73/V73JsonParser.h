// SecretEngine - v7.3 JSON Parser
// Exact v7.3 Ultra Full Specification Compatibility
// YXZ rotation order, degrees, euler format

#pragma once
#include "V73CoreTypes.h"
#include <nlohmann/json.hpp>
#include <expected>
#include <string>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// v7.3 JSON Structures (Exact Specification Match)
// ============================================================================
struct V73LevelManifest {
    std::string version = "7.3";
    std::string name;
    
    struct CoreSettings {
        struct TransformSettings {
            std::string rotation_format = "euler";
            std::string rotation_unit = "degrees";
            std::string rotation_order = "YXZ";
        } transform_settings;
    } core;
    
    struct References {
        std::string players;  // "entities/players.json"
        std::vector<std::string> chunks;  // ["chunks/chunk_0.json", ...]
    } references;
    
    // Optional streaming settings (not in base v7.3 spec)
    struct StreamingSettings {
        float chunk_size = 100.0f;
        int load_radius = 2;
        int unload_radius = 3;
        std::string method = "distance";
    } streaming;
};

struct V73ChunkData {
    std::string chunk_id;
    
    struct MeshGroup {
        std::string mesh;     // "mesh_62.mesh"
        std::string material; // "mesh_62.mat"
        
        struct Instance {
            struct Transform {
                std::array<float, 3> position;
                std::array<float, 3> rotation;  // YXZ degrees
                std::array<float, 3> scale;
            } transform;
            
            struct Tags {
                std::string type = "mesh";
            } tags;
            
            struct MaterialParams {
                std::array<float, 3> color;
            } material_params;
            
            struct Culling {
                float radius = 1.0f;
            } culling;
            
            struct LOD {
                struct Level {
                    float distance;
                };
                std::vector<Level> levels;
            } lod;
        };
        
        std::vector<Instance> instances;
    };
    
    std::vector<MeshGroup> mesh_groups;
};

struct V73PlayerData {
    std::vector<Player> players;
};

// ============================================================================
// Asset Manager Interface
// ============================================================================
class AssetManager {
public:
    virtual ~AssetManager() = default;
    virtual uint32_t GetMeshId(const std::string& path) = 0;
    virtual uint32_t GetMaterialId(const std::string& path) = 0;
    virtual bool LoadMesh(const std::string& path) = 0;
    virtual bool LoadMaterial(const std::string& path) = 0;
};

// ============================================================================
// v7.3 JSON Parser
// ============================================================================
class V73JsonParser {
public:
    V73JsonParser(AssetManager* assetManager = nullptr);
    ~V73JsonParser() = default;
    
    // ========================================================================
    // Main Parsing Functions
    // ========================================================================
    std::expected<V73LevelManifest, std::string> ParseLevelManifest(const std::string& jsonText);
    std::expected<V73ChunkData, std::string> ParseChunkData(const std::string& jsonText);
    std::expected<V73PlayerData, std::string> ParsePlayerData(const std::string& jsonText);
    
    // ========================================================================
    // Conversion Functions (v7.3 → Internal Format)
    // ========================================================================
    std::expected<std::vector<Player>, std::string> ConvertPlayers(const V73PlayerData& v73Data);
    std::expected<std::vector<ChunkInstance>, std::string> ConvertChunkInstances(
        const V73ChunkData& v73Data);
    
    // ========================================================================
    // Utility Functions
    // ========================================================================
    static glm::quat ConvertYXZRotation(const std::array<float, 3>& eulerDegrees);
    static TransformCompact ConvertTransform(const V73ChunkData::MeshGroup::Instance::Transform& v73Transform);
    static LODConfig ConvertLODConfig(const V73ChunkData::MeshGroup::Instance::LOD& v73LOD);
    
    // ========================================================================
    // Validation
    // ========================================================================
    bool ValidateV73Format(const nlohmann::json& json) const;
    std::string GetLastError() const { return m_lastError; }
    
private:
    AssetManager* m_assetManager;
    mutable std::string m_lastError;
    
    // Internal parsing helpers
    std::expected<V73LevelManifest::CoreSettings, std::string> ParseCoreSettings(const nlohmann::json& json);
    std::expected<V73LevelManifest::References, std::string> ParseReferences(const nlohmann::json& json);
    std::expected<V73ChunkData::MeshGroup, std::string> ParseMeshGroup(const nlohmann::json& json);
    std::expected<V73ChunkData::MeshGroup::Instance, std::string> ParseInstance(const nlohmann::json& json);
    std::expected<Player, std::string> ParsePlayer(const nlohmann::json& json);
    
    // Validation helpers
    bool ValidateTransformSettings(const nlohmann::json& json) const;
    bool ValidateVersion(const std::string& version) const;
    
    void SetError(const std::string& error) const { m_lastError = error; }
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline glm::quat V73JsonParser::ConvertYXZRotation(const std::array<float, 3>& eulerDegrees) {
    // Convert degrees to radians
    float yaw = glm::radians(eulerDegrees[1]);    // Y rotation
    float pitch = glm::radians(eulerDegrees[0]);  // X rotation  
    float roll = glm::radians(eulerDegrees[2]);   // Z rotation
    
    // Apply YXZ rotation order (as specified in v7.3)
    glm::quat qY = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
    glm::quat qX = glm::angleAxis(pitch, glm::vec3(1, 0, 0));
    glm::quat qZ = glm::angleAxis(roll, glm::vec3(0, 0, 1));
    
    // YXZ order: first Y, then X, then Z
    return qZ * qX * qY;
}

inline TransformCompact V73JsonParser::ConvertTransform(
    const V73ChunkData::MeshGroup::Instance::Transform& v73Transform) {
    
    TransformCompact transform;
    
    // Position (direct copy)
    transform.position = glm::vec3(
        v73Transform.position[0],
        v73Transform.position[1], 
        v73Transform.position[2]
    );
    
    // Rotation (YXZ degrees → quaternion)
    transform.rotation = ConvertYXZRotation(v73Transform.rotation);
    
    // Scale (direct copy)
    transform.scale = glm::vec3(
        v73Transform.scale[0],
        v73Transform.scale[1],
        v73Transform.scale[2]
    );
    
    return transform;
}

inline LODConfig V73JsonParser::ConvertLODConfig(const V73ChunkData::MeshGroup::Instance::LOD& v73LOD) {
    LODConfig config;
    config.enabled = !v73LOD.levels.empty();
    
    config.levels.reserve(v73LOD.levels.size());
    for (const auto& level : v73LOD.levels) {
        LODLevel lodLevel;
        lodLevel.distance = level.distance;
        lodLevel.meshId = 0;  // Will be set by asset manager
        lodLevel.materialId = 0;  // Will be set by asset manager
        lodLevel.screenSize = 1.0f;  // Default
        config.levels.push_back(lodLevel);
    }
    
    return config;
}

} // namespace SecretEngine::Levels::V73