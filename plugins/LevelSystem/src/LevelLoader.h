// SecretEngine
// Module: LevelLoader
// Responsibility: Load level data from JSON files
// Dependencies: LevelTypes, Core

#pragma once
#include "LevelTypes.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/Components.h>
#include <nlohmann/json.hpp>

namespace SecretEngine::Levels {

class LevelLoader {
public:
    LevelLoader(ICore* core);
    
    // Load level from JSON file
    bool LoadLevelFromFile(const char* levelPath, Level* level);
    
    // Load scene.json format (for compatibility)
    bool LoadSceneFormat(const char* scenePath, Level* level);
    
    // Load v7.3 level format
    bool LoadV73LevelFormat(const nlohmann::json& json, const char* levelPath, Level* level);
    
    // Save level to JSON file
    bool SaveLevelToFile(const char* levelPath, const Level* level);
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    
    // Parse entity from JSON
    Entity ParseEntity(const nlohmann::json& entityData, Level* level);
    
    // Parse transform component
    TransformComponent* ParseTransform(const nlohmann::json& transformData);
    
    // Parse mesh component
    MeshComponent* ParseMesh(const nlohmann::json& meshData);
};

} // namespace SecretEngine::Levels
