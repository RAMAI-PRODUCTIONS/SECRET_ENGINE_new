// SecretEngine - Scene Loader Implementation

#include "SceneLoader.h"
#include <SecretEngine/ComponentsNew.h>
#include <SecretEngine/IAssetProvider.h>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace SecretEngine::Levels {

SceneLoader::SceneLoader(ILogger* logger)
    : m_logger(logger)
{
}

SceneLoader::~SceneLoader() {
}

Scene* SceneLoader::LoadLevelToScene(const std::string& filepath) {
    if (!m_logger) return nullptr;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Loading level into Scene: %s", filepath.c_str());
    m_logger->LogInfo("SceneLoader", msg);
    
    // Read JSON file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        snprintf(msg, sizeof(msg), "Failed to open level file: %s", filepath.c_str());
        m_logger->LogError("SceneLoader", msg);
        return nullptr;
    }
    
    json levelData;
    try {
        file >> levelData;
    } catch (const std::exception& e) {
        snprintf(msg, sizeof(msg), "Failed to parse JSON: %s", e.what());
        m_logger->LogError("SceneLoader", msg);
        return nullptr;
    }
    
    // Create scene
    std::string sceneName = levelData.value("level_id", "Untitled");
    auto* scene = new Scene(sceneName);
    
    m_logger->LogInfo("SceneLoader", "Scene created, loading chunks...");
    
    // Load chunks
    if (levelData.contains("chunks")) {
        for (const auto& chunkRef : levelData["chunks"]) {
            std::string chunkPath = chunkRef.value("path", "");
            if (!chunkPath.empty()) {
                LoadChunkIntoScene(scene, chunkPath);
            }
        }
    }
    
    // Create camera if not present
    bool hasCamera = false;
    for (const auto& entity : scene->GetAllEntities()) {
        if (entity->GetComponent<CameraComponentNew>()) {
            hasCamera = true;
            break;
        }
    }
    
    if (!hasCamera) {
        m_logger->LogInfo("SceneLoader", "No camera found, creating default camera...");
        auto* cameraEntity = scene->CreateEntity("MainCamera");
        auto* cameraTransform = cameraEntity->AddComponent<TransformComponentNew>();
        cameraTransform->SetLocalPosition(Vec3(0, 5, -10));
        cameraTransform->SetLocalRotation(Vec3(15, 0, 0));
        
        auto* camera = cameraEntity->AddComponent<CameraComponentNew>();
        camera->SetMainCamera(true);
        camera->SetFOV(60.0f);
        camera->SetNearPlane(0.1f);
        camera->SetFarPlane(1000.0f);
    }
    
    snprintf(msg, sizeof(msg), "Level loaded successfully: %zu entities created", 
             scene->GetAllEntities().size());
    m_logger->LogInfo("SceneLoader", msg);
    
    return scene;
}

bool SceneLoader::LoadChunkIntoScene(Scene* scene, const std::string& filepath) {
    if (!scene || !m_logger) return false;
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Loading chunk: %s", filepath.c_str());
    m_logger->LogInfo("SceneLoader", msg);
    
    // Read JSON file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        snprintf(msg, sizeof(msg), "Failed to open chunk file: %s", filepath.c_str());
        m_logger->LogError("SceneLoader", msg);
        return false;
    }
    
    json chunkData;
    try {
        file >> chunkData;
    } catch (const std::exception& e) {
        snprintf(msg, sizeof(msg), "Failed to parse chunk JSON: %s", e.what());
        m_logger->LogError("SceneLoader", msg);
        return false;
    }
    
    // Parse mesh groups
    if (!chunkData.contains("mesh_groups")) {
        m_logger->LogWarning("SceneLoader", "Chunk has no mesh_groups");
        return true;
    }
    
    int entityCount = 0;
    
    for (const auto& meshGroup : chunkData["mesh_groups"]) {
        std::string meshPath = meshGroup.value("mesh", "");
        
        if (!meshGroup.contains("instances")) continue;
        
        for (const auto& instance : meshGroup["instances"]) {
            // Create entity
            std::string entityName = "Entity";
            if (instance.contains("tags") && instance["tags"].contains("name")) {
                entityName = instance["tags"]["name"];
            }
            
            auto* entity = scene->CreateEntity(entityName);
            
            // Add transform
            auto* transform = entity->AddComponent<TransformComponentNew>();
            
            if (instance.contains("transform")) {
                const auto& t = instance["transform"];
                
                if (t.contains("position") && t["position"].is_array() && t["position"].size() >= 3) {
                    transform->SetLocalPosition(Vec3(
                        t["position"][0].get<float>(),
                        t["position"][1].get<float>(),
                        t["position"][2].get<float>()
                    ));
                }
                
                if (t.contains("rotation") && t["rotation"].is_array() && t["rotation"].size() >= 3) {
                    transform->SetLocalRotation(Vec3(
                        t["rotation"][0].get<float>(),
                        t["rotation"][1].get<float>(),
                        t["rotation"][2].get<float>()
                    ));
                }
                
                if (t.contains("scale") && t["scale"].is_array() && t["scale"].size() >= 3) {
                    transform->SetLocalScale(Vec3(
                        t["scale"][0].get<float>(),
                        t["scale"][1].get<float>(),
                        t["scale"][2].get<float>()
                    ));
                }
            }
            
            // Add mesh
            auto* mesh = entity->AddComponent<MeshComponentNew>();
            mesh->SetMeshPath(meshPath.c_str());
            mesh->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
            mesh->SetVisible(true);
            
            entityCount++;
        }
    }
    
    snprintf(msg, sizeof(msg), "Chunk loaded: %d entities created", entityCount);
    m_logger->LogInfo("SceneLoader", msg);
    
    return true;
}

} // namespace SecretEngine::Levels
