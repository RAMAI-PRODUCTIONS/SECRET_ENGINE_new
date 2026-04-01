// SecretEngine - v7.3 JSON Parser Implementation
// Exact v7.3 Ultra Full Specification Compatibility

#include "V73JsonParser.h"
#include <sstream>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// Constructor
// ============================================================================
V73JsonParser::V73JsonParser(AssetManager* assetManager) 
    : m_assetManager(assetManager) {
}

// ============================================================================
// Main Parsing Functions
// ============================================================================
std::expected<V73LevelManifest, std::string> V73JsonParser::ParseLevelManifest(const std::string& jsonText) {
    try {
        nlohmann::json json = nlohmann::json::parse(jsonText);
        
        if (!ValidateV73Format(json)) {
            return std::unexpected("Invalid v7.3 format: " + m_lastError);
        }
        
        V73LevelManifest manifest;
        
        // Required fields
        if (!json.contains("version")) {
            return std::unexpected("Missing required field: version");
        }
        manifest.version = json["version"].get<std::string>();
        
        if (!ValidateVersion(manifest.version)) {
            return std::unexpected("Unsupported version: " + manifest.version);
        }
        
        if (!json.contains("name")) {
            return std::unexpected("Missing required field: name");
        }
        manifest.name = json["name"].get<std::string>();
        
        // Core settings
        if (json.contains("core")) {
            auto coreResult = ParseCoreSettings(json["core"]);
            if (!coreResult) {
                return std::unexpected("Failed to parse core settings: " + coreResult.error());
            }
            manifest.core = *coreResult;
        }
        
        // References
        if (json.contains("references")) {
            auto refResult = ParseReferences(json["references"]);
            if (!refResult) {
                return std::unexpected("Failed to parse references: " + refResult.error());
            }
            manifest.references = *refResult;
        }
        
        // Optional streaming settings (extension to v7.3)
        if (json.contains("streaming")) {
            const auto& streaming = json["streaming"];
            if (streaming.contains("chunk_size")) {
                manifest.streaming.chunk_size = streaming["chunk_size"].get<float>();
            }
            if (streaming.contains("load_radius")) {
                manifest.streaming.load_radius = streaming["load_radius"].get<int>();
            }
            if (streaming.contains("unload_radius")) {
                manifest.streaming.unload_radius = streaming["unload_radius"].get<int>();
            }
            if (streaming.contains("method")) {
                manifest.streaming.method = streaming["method"].get<std::string>();
            }
        }
        
        return manifest;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected("JSON parsing error: " + std::string(e.what()));
    }
}

std::expected<V73ChunkData, std::string> V73JsonParser::ParseChunkData(const std::string& jsonText) {
    try {
        nlohmann::json json = nlohmann::json::parse(jsonText);
        
        V73ChunkData chunkData;
        
        // Required fields
        if (!json.contains("chunk_id")) {
            return std::unexpected("Missing required field: chunk_id");
        }
        chunkData.chunk_id = json["chunk_id"].get<std::string>();
        
        // Parse mesh groups
        if (json.contains("mesh_groups")) {
            const auto& meshGroups = json["mesh_groups"];
            chunkData.mesh_groups.reserve(meshGroups.size());
            
            for (const auto& groupJson : meshGroups) {
                auto groupResult = ParseMeshGroup(groupJson);
                if (!groupResult) {
                    return std::unexpected("Failed to parse mesh group: " + groupResult.error());
                }
                chunkData.mesh_groups.push_back(*groupResult);
            }
        }
        
        return chunkData;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected("JSON parsing error: " + std::string(e.what()));
    }
}

std::expected<V73PlayerData, std::string> V73JsonParser::ParsePlayerData(const std::string& jsonText) {
    try {
        nlohmann::json json = nlohmann::json::parse(jsonText);
        
        V73PlayerData playerData;
        
        if (json.contains("players")) {
            const auto& playersJson = json["players"];
            playerData.players.reserve(playersJson.size());
            
            for (const auto& playerJson : playersJson) {
                auto playerResult = ParsePlayer(playerJson);
                if (!playerResult) {
                    return std::unexpected("Failed to parse player: " + playerResult.error());
                }
                playerData.players.push_back(*playerResult);
            }
        }
        
        return playerData;
        
    } catch (const nlohmann::json::exception& e) {
        return std::unexpected("JSON parsing error: " + std::string(e.what()));
    }
}

// ============================================================================
// Conversion Functions
// ============================================================================
std::expected<std::vector<Player>, std::string> V73JsonParser::ConvertPlayers(const V73PlayerData& v73Data) {
    std::vector<Player> players;
    players.reserve(v73Data.players.size());
    
    for (const auto& v73Player : v73Data.players) {
        players.push_back(v73Player);  // Direct copy since Player is already in internal format
    }
    
    return players;
}

std::expected<std::vector<ChunkInstance>, std::string> V73JsonParser::ConvertChunkInstances(
    const V73ChunkData& v73Data) {
    
    std::vector<ChunkInstance> instances;
    
    for (const auto& meshGroup : v73Data.mesh_groups) {
        // Get mesh and material IDs from asset manager
        uint32_t meshId = 0;
        uint32_t materialId = 0;
        
        if (m_assetManager) {
            meshId = m_assetManager->GetMeshId(meshGroup.mesh);
            materialId = m_assetManager->GetMaterialId(meshGroup.material);
            
            // Ensure assets are loaded
            if (!m_assetManager->LoadMesh(meshGroup.mesh)) {
                return std::unexpected("Failed to load mesh: " + meshGroup.mesh);
            }
            if (!m_assetManager->LoadMaterial(meshGroup.material)) {
                return std::unexpected("Failed to load material: " + meshGroup.material);
            }
        }
        
        // Convert each instance
        for (const auto& v73Instance : meshGroup.instances) {
            ChunkInstance instance;
            
            // Transform conversion (YXZ degrees → internal format)
            instance.transform = ConvertTransform(v73Instance.transform);
            
            // Asset references
            instance.baseMeshId = meshId;
            instance.baseMaterialId = materialId;
            
            // Material parameters
            instance.colorOverride = glm::vec4(
                v73Instance.material_params.color[0],
                v73Instance.material_params.color[1],
                v73Instance.material_params.color[2],
                1.0f
            );
            
            // LOD configuration
            instance.lodConfig = ConvertLODConfig(v73Instance.lod);
            
            // Update LOD mesh/material IDs if asset manager is available
            if (m_assetManager) {
                for (auto& lodLevel : instance.lodConfig.levels) {
                    lodLevel.meshId = meshId;  // In production, would have LOD-specific meshes
                    lodLevel.materialId = materialId;
                }
            }
            
            // Culling configuration
            instance.cullingConfig.enabled = true;
            instance.cullingConfig.method = CullingConfig::Method::Sphere;
            instance.cullingConfig.radius = v73Instance.culling.radius;
            
            instances.push_back(instance);
        }
    }
    
    return instances;
}

// ============================================================================
// Internal Parsing Helpers
// ============================================================================
std::expected<V73LevelManifest::CoreSettings, std::string> V73JsonParser::ParseCoreSettings(const nlohmann::json& json) {
    V73LevelManifest::CoreSettings core;
    
    if (json.contains("transform_settings")) {
        const auto& transform = json["transform_settings"];
        
        if (transform.contains("rotation_format")) {
            core.transform_settings.rotation_format = transform["rotation_format"].get<std::string>();
        }
        if (transform.contains("rotation_unit")) {
            core.transform_settings.rotation_unit = transform["rotation_unit"].get<std::string>();
        }
        if (transform.contains("rotation_order")) {
            core.transform_settings.rotation_order = transform["rotation_order"].get<std::string>();
        }
        
        // Validate transform settings
        if (!ValidateTransformSettings(transform)) {
            return std::unexpected("Invalid transform settings: " + m_lastError);
        }
    }
    
    return core;
}

std::expected<V73LevelManifest::References, std::string> V73JsonParser::ParseReferences(const nlohmann::json& json) {
    V73LevelManifest::References references;
    
    if (json.contains("players")) {
        references.players = json["players"].get<std::string>();
    }
    
    if (json.contains("chunks")) {
        const auto& chunks = json["chunks"];
        references.chunks.reserve(chunks.size());
        
        for (const auto& chunk : chunks) {
            references.chunks.push_back(chunk.get<std::string>());
        }
    }
    
    return references;
}

std::expected<V73ChunkData::MeshGroup, std::string> V73JsonParser::ParseMeshGroup(const nlohmann::json& json) {
    V73ChunkData::MeshGroup group;
    
    // Required fields
    if (!json.contains("mesh")) {
        return std::unexpected("Missing required field: mesh");
    }
    group.mesh = json["mesh"].get<std::string>();
    
    if (!json.contains("material")) {
        return std::unexpected("Missing required field: material");
    }
    group.material = json["material"].get<std::string>();
    
    // Parse instances
    if (json.contains("instances")) {
        const auto& instances = json["instances"];
        group.instances.reserve(instances.size());
        
        for (const auto& instanceJson : instances) {
            auto instanceResult = ParseInstance(instanceJson);
            if (!instanceResult) {
                return std::unexpected("Failed to parse instance: " + instanceResult.error());
            }
            group.instances.push_back(*instanceResult);
        }
    }
    
    return group;
}

std::expected<V73ChunkData::MeshGroup::Instance, std::string> V73JsonParser::ParseInstance(const nlohmann::json& json) {
    V73ChunkData::MeshGroup::Instance instance;
    
    // Transform (required)
    if (!json.contains("transform")) {
        return std::unexpected("Missing required field: transform");
    }
    
    const auto& transform = json["transform"];
    
    if (!transform.contains("position") || !transform.contains("rotation") || !transform.contains("scale")) {
        return std::unexpected("Transform missing required fields (position, rotation, scale)");
    }
    
    // Position
    const auto& pos = transform["position"];
    if (pos.size() != 3) {
        return std::unexpected("Position must have exactly 3 components");
    }
    instance.transform.position = {pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>()};
    
    // Rotation (YXZ degrees)
    const auto& rot = transform["rotation"];
    if (rot.size() != 3) {
        return std::unexpected("Rotation must have exactly 3 components");
    }
    instance.transform.rotation = {rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>()};
    
    // Scale
    const auto& scale = transform["scale"];
    if (scale.size() != 3) {
        return std::unexpected("Scale must have exactly 3 components");
    }
    instance.transform.scale = {scale[0].get<float>(), scale[1].get<float>(), scale[2].get<float>()};
    
    // Tags (optional)
    if (json.contains("tags")) {
        const auto& tags = json["tags"];
        if (tags.contains("type")) {
            instance.tags.type = tags["type"].get<std::string>();
        }
    }
    
    // Material parameters (optional)
    if (json.contains("material_params")) {
        const auto& matParams = json["material_params"];
        if (matParams.contains("color")) {
            const auto& color = matParams["color"];
            if (color.size() >= 3) {
                instance.material_params.color = {
                    color[0].get<float>(), 
                    color[1].get<float>(), 
                    color[2].get<float>()
                };
            }
        }
    }
    
    // Culling (optional)
    if (json.contains("culling")) {
        const auto& culling = json["culling"];
        if (culling.contains("radius")) {
            instance.culling.radius = culling["radius"].get<float>();
        }
    }
    
    // LOD (optional)
    if (json.contains("lod")) {
        const auto& lod = json["lod"];
        if (lod.contains("levels")) {
            const auto& levels = lod["levels"];
            instance.lod.levels.reserve(levels.size());
            
            for (const auto& level : levels) {
                V73ChunkData::MeshGroup::Instance::LOD::Level lodLevel;
                if (level.contains("distance")) {
                    lodLevel.distance = level["distance"].get<float>();
                }
                instance.lod.levels.push_back(lodLevel);
            }
        }
    }
    
    return instance;
}

std::expected<Player, std::string> V73JsonParser::ParsePlayer(const nlohmann::json& json) {
    Player player;
    
    // Required fields
    if (!json.contains("id")) {
        return std::unexpected("Missing required field: id");
    }
    
    std::string idStr = json["id"].get<std::string>();
    // Extract numeric ID from string like "player_0"
    size_t underscorePos = idStr.find('_');
    if (underscorePos != std::string::npos) {
        player.id = static_cast<uint32_t>(std::stoul(idStr.substr(underscorePos + 1)));
    } else {
        player.id = static_cast<uint32_t>(std::stoul(idStr));
    }
    
    // Transform
    if (json.contains("transform")) {
        const auto& transform = json["transform"];
        
        if (transform.contains("position")) {
            const auto& pos = transform["position"];
            player.transform.position = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
        }
        
        if (transform.contains("rotation")) {
            const auto& rot = transform["rotation"];
            std::array<float, 3> eulerDegrees = {rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>()};
            player.transform.rotation = ConvertYXZRotation(eulerDegrees);
        }
        
        if (transform.contains("scale")) {
            const auto& scale = transform["scale"];
            player.transform.scale = glm::vec3(scale[0].get<float>(), scale[1].get<float>(), scale[2].get<float>());
        }
    }
    
    // Stats
    if (json.contains("stats")) {
        const auto& stats = json["stats"];
        if (stats.contains("health")) {
            player.stats.health = stats["health"].get<int32_t>();
        }
        if (stats.contains("stamina")) {
            player.stats.stamina = stats["stamina"].get<int32_t>();
        }
    }
    
    // Loadout
    if (json.contains("loadout")) {
        const auto& loadout = json["loadout"];
        if (loadout.contains("primary")) {
            player.loadout.primary = loadout["primary"].get<std::string>();
        }
        if (loadout.contains("secondary")) {
            player.loadout.secondary = loadout["secondary"].get<std::string>();
        }
        if (loadout.contains("tertiary")) {
            player.loadout.tertiary = loadout["tertiary"].get<std::string>();
        }
    }
    
    // Skill tree
    if (json.contains("skill_tree") && json["skill_tree"].contains("nodes")) {
        const auto& nodes = json["skill_tree"]["nodes"];
        
        for (size_t i = 0; i < nodes.size() && i < player.stats.skillTree.size(); ++i) {
            const auto& node = nodes[i];
            if (node.contains("id")) {
                player.stats.skillTree[i].id = node["id"].get<std::string>();
            }
            if (node.contains("level")) {
                player.stats.skillTree[i].level = node["level"].get<int32_t>();
            }
        }
    }
    
    return player;
}

// ============================================================================
// Validation Functions
// ============================================================================
bool V73JsonParser::ValidateV73Format(const nlohmann::json& json) const {
    // Check required top-level fields
    if (!json.contains("version")) {
        SetError("Missing version field");
        return false;
    }
    
    if (!json.contains("name")) {
        SetError("Missing name field");
        return false;
    }
    
    // Validate version
    std::string version = json["version"].get<std::string>();
    if (!ValidateVersion(version)) {
        return false;
    }
    
    // Validate core settings if present
    if (json.contains("core") && json["core"].contains("transform_settings")) {
        if (!ValidateTransformSettings(json["core"]["transform_settings"])) {
            return false;
        }
    }
    
    return true;
}

bool V73JsonParser::ValidateTransformSettings(const nlohmann::json& json) const {
    if (json.contains("rotation_format")) {
        std::string format = json["rotation_format"].get<std::string>();
        if (format != "euler" && format != "quaternion") {
            SetError("Invalid rotation_format: " + format + " (must be 'euler' or 'quaternion')");
            return false;
        }
    }
    
    if (json.contains("rotation_unit")) {
        std::string unit = json["rotation_unit"].get<std::string>();
        if (unit != "degrees" && unit != "radians") {
            SetError("Invalid rotation_unit: " + unit + " (must be 'degrees' or 'radians')");
            return false;
        }
    }
    
    if (json.contains("rotation_order")) {
        std::string order = json["rotation_order"].get<std::string>();
        if (order != "XYZ" && order != "XZY" && order != "YXZ" && 
            order != "YZX" && order != "ZXY" && order != "ZYX") {
            SetError("Invalid rotation_order: " + order + " (must be XYZ, XZY, YXZ, YZX, ZXY, or ZYX)");
            return false;
        }
    }
    
    return true;
}

bool V73JsonParser::ValidateVersion(const std::string& version) const {
    if (version != "7.3") {
        SetError("Unsupported version: " + version + " (only 7.3 is supported)");
        return false;
    }
    return true;
}

} // namespace SecretEngine::Levels::V73