// SecretEngine
// Module: LevelManager
// Responsibility: Level loading, unloading, and streaming
// Dependencies: LevelTypes

#pragma once
#include "LevelTypes.h"
#include "LevelLoader.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <nlohmann/json.hpp>

namespace SecretEngine::Levels {

class LevelManager {
public:
    static constexpr int MAX_LEVELS = 32;
    
    LevelManager(ICore* core);
    ~LevelManager();
    
    // Level Loading
    bool LoadLevelDefinitions(const char* path);
    bool LoadLevel(const char* levelName, LoadPriority priority = LoadPriority::Normal);
    bool UnloadLevel(const char* levelName);
    bool ReloadLevel(const char* levelName);
    void UnloadCurrentLevel(); // Unload all non-persistent levels
    
    // Level Visibility
    void ShowLevel(const char* levelName);
    void HideLevel(const char* levelName);
    void ToggleLevelVisibility(const char* levelName);
    
    // Level Queries
    Level* GetLevel(const char* levelName);
    Level* GetLevelByIndex(int index);
    int GetLevelIndex(const char* levelName) const;
    LevelState GetLevelState(const char* levelName) const;
    bool IsLevelLoaded(const char* levelName) const;
    bool IsLevelVisible(const char* levelName) const;
    
    // Persistent Level
    void SetPersistentLevel(const char* levelName);
    Level* GetPersistentLevel();
    
    // Level Streaming
    void UpdateStreaming(float dt, const float playerPosition[3]);
    void EnableStreaming(bool enable) { m_streamingEnabled = enable; }
    bool IsStreamingEnabled() const { return m_streamingEnabled; }
    
    // Level Transitions
    void TransitionToLevel(const char* levelName, const LevelTransition& transition);
    bool IsTransitioning() const { return m_isTransitioning; }
    float GetTransitionProgress() const { return m_transitionProgress; }
    
    // Sub-levels
    bool LoadSubLevels(const char* parentLevelName);
    bool UnloadSubLevels(const char* parentLevelName);
    
    // Entity Management
    void AddEntityToLevel(uint32_t entityId, const char* levelName);
    void RemoveEntityFromLevel(uint32_t entityId);
    const char* GetEntityLevel(uint32_t entityId) const;
    
    // Level Volumes
    void RegisterStreamingVolume(const LevelStreamingVolume& volume);
    void UpdateStreamingVolumes(const float playerPosition[3]);
    
    // Utilities
    void UnloadAllLevels();
    void GetLoadedLevels(char levelNames[][64], int& count, int maxCount) const;
    int GetLoadedLevelCount() const;
    
    // Debug
    void PrintLevelInfo() const;
    void PrintLevelHierarchy() const;
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    LevelLoader* m_levelLoader;
    
    Level m_levels[MAX_LEVELS];
    int m_levelCount = 0;
    
    int m_persistentLevelIndex = -1;
    bool m_streamingEnabled = true;
    
    // Transition state
    bool m_isTransitioning = false;
    float m_transitionProgress = 0.0f;
    LevelTransition m_currentTransition;
    
    // Streaming volumes
    LevelStreamingVolume m_streamingVolumes[32];
    int m_volumeCount = 0;
    
    // Internal helpers
    int FindFreeLevelSlot();
    void LoadLevelData(Level* level);
    void UnloadLevelData(Level* level);
    void UpdateLevelState(Level* level, float dt);
    void CheckDistanceStreaming(const float playerPosition[3]);
    void CheckVolumeStreaming(const float playerPosition[3]);
    
    // JSON parsing
    LevelDefinition ParseLevelDefinition(const nlohmann::json& json);
};

} // namespace SecretEngine::Levels
