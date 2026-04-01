// SecretEngine
// Module: LevelLoader
// Responsibility: Load level data from JSON files
// Dependencies: LevelLoader.h

#include "LevelLoader.h"
#include <SecretEngine/IAssetProvider.h>
#include <fstream>

namespace SecretEngine::Levels {

LevelLoader::LevelLoader(ICore* core)
    : m_core(core)
    , m_world(core->GetWorld())
    , m_logger(core->GetLogger())
{
}

bool LevelLoader::LoadLevelFromFile(const char* levelPath, Level* level) {
    char msg[256];
    snprintf(msg, sizeof(msg), "🔍 Attempting to load level file: %s", levelPath);
    m_logger->LogInfo("LevelLoader", msg);
    
    // Use AssetProvider for cross-platform asset loading (works on Android)
    auto assetProvider = m_core->GetAssetProvider();
    if (!assetProvider) {
        m_logger->LogError("LevelLoader", "❌ AssetProvider not available");
        return false;
    }
    
    std::string jsonText = assetProvider->LoadText(levelPath);
    if (jsonText.empty()) {
        snprintf(msg, sizeof(msg), "❌ Failed to load level file: %s (file not found or empty)", levelPath);
        m_logger->LogError("LevelLoader", msg);
        
        // Try alternative paths for debugging
        snprintf(msg, sizeof(msg), "💡 Tried path: %s", levelPath);
        m_logger->LogInfo("LevelLoader", msg);
        return false;
    }
    
    snprintf(msg, sizeof(msg), "✅ File loaded successfully (%zu bytes), parsing JSON...", jsonText.size());
    m_logger->LogInfo("LevelLoader", msg);
    
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonText);
        m_logger->LogInfo("LevelLoader", "✅ JSON parsed successfully");
    } catch (const std::exception& e) {
        snprintf(msg, sizeof(msg), "❌ JSON parse error: %s", e.what());
        m_logger->LogError("LevelLoader", msg);
        return false;
    }
    
    // Check if it's scene.json format or level format
    if (json.contains("entities") && json["entities"].is_array()) {
        snprintf(msg, sizeof(msg), "📦 Detected scene format (entities array) with %zu entities", 
                 json["entities"].size());
        m_logger->LogInfo("LevelLoader", msg);
        return LoadSceneFormat(levelPath, level);
    }
    
    // Check if it's v7.3 level format (has references section)
    if (json.contains("references") || json.contains("version")) {
        m_logger->LogInfo("LevelLoader", "📦 Detected v7.3 level format");
        return LoadV73LevelFormat(json, levelPath, level);
    }
    
    // TODO: Implement custom level format
    m_logger->LogWarning("LevelLoader", "⚠️ Custom level format not yet implemented");
    return false;
}

bool LevelLoader::LoadSceneFormat(const char* scenePath, Level* level) {
    // Use AssetProvider for cross-platform asset loading
    auto assetProvider = m_core->GetAssetProvider();
    if (!assetProvider) {
        m_logger->LogError("LevelLoader", "AssetProvider not available");
        return false;
    }
    
    std::string jsonText = assetProvider->LoadText(scenePath);
    if (jsonText.empty()) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed to load scene file: %s", scenePath);
        m_logger->LogError("LevelLoader", msg);
        return false;
    }
    
    nlohmann::json data;
    try {
        data = nlohmann::json::parse(jsonText);
    } catch (const std::exception& e) {
        char msg[256];
        snprintf(msg, sizeof(msg), "JSON parse error in scene file: %s", e.what());
        m_logger->LogError("LevelLoader", msg);
        return false;
    }
    
    if (!data.contains("entities") || !data["entities"].is_array()) {
        m_logger->LogError("LevelLoader", "Invalid scene format: missing 'entities' array");
        return false;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loading scene: %s", scenePath);
    m_logger->LogInfo("LevelLoader", msg);
    
    int entityCount = 0;
    for (const auto& entityData : data["entities"]) {
        Entity e = ParseEntity(entityData, level);
        if (e.id != Entity::Invalid.id) {
            level->AddEntity(e.id);
            entityCount++;
        }
    }
    
    snprintf(msg, sizeof(msg), "Loaded %d entities from scene", entityCount);
    m_logger->LogInfo("LevelLoader", msg);
    
    return true;
}

Entity LevelLoader::ParseEntity(const nlohmann::json& entityData, Level* level) {
    Entity e = m_world->CreateEntity();
    
    // Parse name
    if (entityData.contains("name")) {
        std::string name = entityData["name"].get<std::string>();
        // TODO: Add name component if needed
    }
    
    // Parse transform
    std::string transformKey = entityData.contains("transformed") ? "transformed" : "transform";
    if (entityData.contains(transformKey)) {
        auto* transform = ParseTransform(entityData[transformKey]);
        if (transform) {
            m_world->AddComponent(e, TransformComponent::TypeID, transform);
        }
    }
    
    // Parse mesh
    if (entityData.contains("mesh")) {
        auto* mesh = ParseMesh(entityData["mesh"]);
        if (mesh) {
            m_world->AddComponent(e, MeshComponent::TypeID, mesh);
        }
    }
    
    // Add level component to track which level this entity belongs to
    auto* levelComp = new LevelComponent();
    strncpy(levelComp->levelName, level->definition.name, 64);
    levelComp->levelIndex = -1;  // Will be set by LevelManager
    m_world->AddComponent(e, LevelComponent::TypeID, levelComp);
    
    return e;
}

TransformComponent* LevelLoader::ParseTransform(const nlohmann::json& transformData) {
    auto* transform = new TransformComponent();
    
    // Position
    if (transformData.contains("position") && transformData["position"].is_array()) {
        auto pos = transformData["position"];
        transform->position[0] = pos.size() > 0 ? pos[0].get<float>() : 0.0f;
        transform->position[1] = pos.size() > 1 ? pos[1].get<float>() : 0.0f;
        transform->position[2] = pos.size() > 2 ? pos[2].get<float>() : 0.0f;
    }
    
    // Rotation (convert degrees to radians)
    if (transformData.contains("rotation") && transformData["rotation"].is_array()) {
        auto rot = transformData["rotation"];
        const float DEG_TO_RAD = 3.14159265f / 180.0f;
        transform->rotation[0] = rot.size() > 0 ? rot[0].get<float>() * DEG_TO_RAD : 0.0f;
        transform->rotation[1] = rot.size() > 1 ? rot[1].get<float>() * DEG_TO_RAD : 0.0f;
        transform->rotation[2] = rot.size() > 2 ? rot[2].get<float>() * DEG_TO_RAD : 0.0f;
    }
    
    // Scale
    if (transformData.contains("scale") && transformData["scale"].is_array()) {
        auto scale = transformData["scale"];
        transform->scale[0] = scale.size() > 0 ? scale[0].get<float>() : 1.0f;
        transform->scale[1] = scale.size() > 1 ? scale[1].get<float>() : 1.0f;
        transform->scale[2] = scale.size() > 2 ? scale[2].get<float>() : 1.0f;
    }
    
    return transform;
}

MeshComponent* LevelLoader::ParseMesh(const nlohmann::json& meshData) {
    auto* mesh = new MeshComponent();
    
    // Mesh path
    if (meshData.contains("path")) {
        std::string meshPath = meshData["path"].get<std::string>();
        strncpy(mesh->meshPath, meshPath.c_str(), 255);
        mesh->meshPath[255] = '\0';
    }
    
    // Color
    if (meshData.contains("color") && meshData["color"].is_array()) {
        auto color = meshData["color"];
        mesh->color[0] = color.size() > 0 ? color[0].get<float>() : 1.0f;
        mesh->color[1] = color.size() > 1 ? color[1].get<float>() : 1.0f;
        mesh->color[2] = color.size() > 2 ? color[2].get<float>() : 1.0f;
        mesh->color[3] = color.size() > 3 ? color[3].get<float>() : 1.0f;
    }
    
    // Texture
    if (meshData.contains("texture")) {
        std::string texturePath = meshData["texture"].get<std::string>();
        strncpy(mesh->texturePath, texturePath.c_str(), 255);
        mesh->texturePath[255] = '\0';
    }
    
    // Normal map
    if (meshData.contains("normalMap")) {
        std::string normalMapPath = meshData["normalMap"].get<std::string>();
        strncpy(mesh->normalMapPath, normalMapPath.c_str(), 255);
        mesh->normalMapPath[255] = '\0';
    }
    
    return mesh;
}

bool LevelLoader::SaveLevelToFile(const char* levelPath, const Level* level) {
    // TODO: Implement level saving
    m_logger->LogWarning("LevelLoader", "Level saving not yet implemented");
    return false;
}

} // namespace SecretEngine::Levels


bool SecretEngine::Levels::LevelLoader::LoadV73LevelFormat(const nlohmann::json& json, const char* levelPath, Level* level) {
    auto assetProvider = m_core->GetAssetProvider();
    if (!assetProvider) {
        m_logger->LogError("LevelLoader", "AssetProvider not available");
        return false;
    }
    
    char msg[256];
    int entityCount = 0;
    
    // Check if it has references section (v7.3 format)
    if (json.contains("references")) {
        const auto& refs = json["references"];
        
        // Load players from references
        if (refs.contains("players") && refs["players"].is_string()) {
            std::string playersFile = refs["players"].get<std::string>();
            // Prepend "levels/" if not already there
            if (playersFile.find("levels/") == std::string::npos) {
                playersFile = "levels/" + playersFile;
            }
            
            snprintf(msg, sizeof(msg), "Loading players from: %s", playersFile.c_str());
            m_logger->LogInfo("LevelLoader", msg);
            
            std::string playersJson = assetProvider->LoadText(playersFile.c_str());
            if (!playersJson.empty()) {
                try {
                    nlohmann::json playersData = nlohmann::json::parse(playersJson);
                    if (playersData.contains("players") && playersData["players"].is_array()) {
                        for (const auto& playerData : playersData["players"]) {
                            Entity e = ParseEntity(playerData, level);
                            if (e.id != Entity::Invalid.id) {
                                level->AddEntity(e.id);
                                entityCount++;
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    snprintf(msg, sizeof(msg), "Failed to parse players file: %s", e.what());
                    m_logger->LogError("LevelLoader", msg);
                }
            }
        }
        
        // Load chunks from references
        if (refs.contains("chunks") && refs["chunks"].is_array()) {
            for (const auto& chunkFile : refs["chunks"]) {
                if (chunkFile.is_string()) {
                    std::string chunkPath = chunkFile.get<std::string>();
                    
                    // Make path relative to level file directory
                    // Extract directory from levelPath
                    std::string levelDir = levelPath;
                    size_t lastSlash = levelDir.find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        levelDir = levelDir.substr(0, lastSlash + 1);
                        chunkPath = levelDir + chunkPath;
                    }
                    
                    snprintf(msg, sizeof(msg), "Loading chunk from: %s", chunkPath.c_str());
                    m_logger->LogInfo("LevelLoader", msg);
                    
                    std::string chunkJson = assetProvider->LoadText(chunkPath.c_str());
                    if (!chunkJson.empty()) {
                        try {
                            nlohmann::json chunkData = nlohmann::json::parse(chunkJson);
                            
                            // Handle mesh_groups format (v7.3)
                            if (chunkData.contains("mesh_groups") && chunkData["mesh_groups"].is_array()) {
                                for (const auto& meshGroup : chunkData["mesh_groups"]) {
                                    if (meshGroup.contains("instances") && meshGroup["instances"].is_array()) {
                                        for (const auto& instance : meshGroup["instances"]) {
                                            // Create entity data in the format ParseEntity expects
                                            nlohmann::json entityData;
                                            
                                            // Copy transform
                                            if (instance.contains("transform")) {
                                                entityData["transform"] = instance["transform"];
                                            }
                                            
                                            // Copy mesh data from group and instance
                                            if (meshGroup.contains("mesh")) {
                                                nlohmann::json meshData;
                                                meshData["path"] = meshGroup["mesh"].get<std::string>();
                                                
                                                // Copy material from group if present
                                                if (meshGroup.contains("material")) {
                                                    meshData["material"] = meshGroup["material"];
                                                }
                                                
                                                // Copy material_params.color to mesh.color
                                                if (instance.contains("material_params")) {
                                                    const auto& matParams = instance["material_params"];
                                                    if (matParams.contains("color") && matParams["color"].is_array()) {
                                                        meshData["color"] = matParams["color"];
                                                    }
                                                    // Store full material_params for future use
                                                    meshData["material_params"] = matParams;
                                                }
                                                
                                                // Copy texture if present in instance
                                                if (instance.contains("texture")) {
                                                    meshData["texture"] = instance["texture"];
                                                }
                                                
                                                entityData["mesh"] = meshData;
                                            }
                                            
                                            // Copy tags
                                            if (instance.contains("tags")) {
                                                const auto& tags = instance["tags"];
                                                if (tags.contains("name")) {
                                                    entityData["name"] = tags["name"];
                                                }
                                                if (tags.contains("type")) {
                                                    entityData["type"] = tags["type"];
                                                }
                                                // Store full tags for future use
                                                entityData["tags"] = tags;
                                            }
                                            
                                            // Copy culling data
                                            if (instance.contains("culling")) {
                                                entityData["culling"] = instance["culling"];
                                            }
                                            
                                            // Copy LOD data
                                            if (instance.contains("lod")) {
                                                entityData["lod"] = instance["lod"];
                                            }
                                            
                                            // Copy physics data if present
                                            if (instance.contains("physics")) {
                                                entityData["physics"] = instance["physics"];
                                            }
                                            
                                            // Copy any custom data
                                            if (instance.contains("custom_data")) {
                                                entityData["custom_data"] = instance["custom_data"];
                                            }
                                            
                                            Entity e = ParseEntity(entityData, level);
                                            if (e.id != Entity::Invalid.id) {
                                                level->AddEntity(e.id);
                                                entityCount++;
                                            }
                                        }
                                    }
                                }
                            }
                            // Handle old meshes format (fallback)
                            else if (chunkData.contains("meshes") && chunkData["meshes"].is_array()) {
                                for (const auto& meshData : chunkData["meshes"]) {
                                    Entity e = ParseEntity(meshData, level);
                                    if (e.id != Entity::Invalid.id) {
                                        level->AddEntity(e.id);
                                        entityCount++;
                                    }
                                }
                            }
                        } catch (const std::exception& e) {
                            snprintf(msg, sizeof(msg), "Failed to parse chunk file: %s", e.what());
                            m_logger->LogError("LevelLoader", msg);
                        }
                    }
                }
            }
        }
    }
    
    snprintf(msg, sizeof(msg), "Loaded %d entities from v7.3 level", entityCount);
    m_logger->LogInfo("LevelLoader", msg);
    
    return entityCount > 0;
}
