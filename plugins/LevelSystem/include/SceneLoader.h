// SecretEngine - Scene Loader for New ECS System
// Loads JSON levels into Scene with EntityObjects

#pragma once
#include <SecretEngine/Scene.h>
#include <SecretEngine/ILogger.h>
#include <string>

namespace SecretEngine::Levels {

/**
 * SceneLoader loads JSON level files into the new ECS Scene system
 */
class SceneLoader {
public:
    explicit SceneLoader(ILogger* logger);
    ~SceneLoader();
    
    /**
     * Load a level JSON file into a Scene
     * @param filepath Path to level JSON file
     * @return Loaded scene or nullptr on failure
     */
    Scene* LoadLevelToScene(const std::string& filepath);
    
    /**
     * Load a chunk JSON file into an existing Scene
     * @param scene Scene to load into
     * @param filepath Path to chunk JSON file
     * @return true on success
     */
    bool LoadChunkIntoScene(Scene* scene, const std::string& filepath);
    
private:
    ILogger* m_logger;
    
    bool LoadChunkData(Scene* scene, const char* jsonData, size_t dataSize);
    void CreateEntityFromInstance(Scene* scene, const void* instanceData);
};

} // namespace SecretEngine::Levels
