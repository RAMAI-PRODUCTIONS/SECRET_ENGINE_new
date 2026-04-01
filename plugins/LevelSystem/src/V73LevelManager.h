// SecretEngine - v7.3 Ultra Full Level Manager
// Complete level management system with multi-disciplinary approach

#pragma once
#include "V73LevelSystem.h"
#include "TriggerSystem.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IAssetProvider.h>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// Level Loading States
// ============================================================================
enum class LevelState {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Unloading,
    Error
};

enum class ChunkState {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Streaming,
    Unloading
};

// ============================================================================
// Performance Metrics
// ============================================================================
struct PerformanceMetrics {
    // Memory usage
    size_t total_memory_usage = 0;
    size_t texture_memory_usage = 0;
    size_t mesh_memory_usage = 0;
    size_t audio_memory_usage = 0;
    
    // Rendering stats
    uint32_t visible_instances = 0;
    uint32_t draw_calls = 0;
    uint32_t triangles_rendered = 0;
    float gpu_frame_time = 0.0f;
    
    // Streaming stats
    uint32_t chunks_loaded = 0;
    uint32_t chunks_streaming = 0;
    float streaming_bandwidth = 0.0f;
    
    // Network stats (for multiplayer)
    uint32_t network_players = 0;
    float network_latency = 0.0f;
    float packet_loss = 0.0f;
    
    void Reset() {
        *this = PerformanceMetrics{};
    }
};

// ============================================================================
// Event System
// ============================================================================
struct LevelEvent {
    enum Type {
        LevelLoaded,
        LevelUnloaded,
        ChunkLoaded,
        ChunkUnloaded,
        PlayerSpawned,
        PlayerDespawned,
        ObjectiveCompleted,
        WeatherChanged,
        TimeOfDayChanged
    };
    
    Type type;
    std::string level_id;
    std::string chunk_id;
    uint32_t player_id = 0;
    nlohmann::json data;
};

using LevelEventCallback = std::function<void(const LevelEvent&)>;

// ============================================================================
// Streaming System
// ============================================================================
class StreamingManager {
public:
    StreamingManager(ICore* core);
    ~StreamingManager();
    
    void SetPlayerPosition(const glm::vec3& position);
    void UpdateStreaming(float deltaTime);
    
    void SetStreamingRadius(float loadRadius, float unloadRadius);
    void SetChunkSize(float size) { m_chunkSize = size; }
    
    bool ShouldLoadChunk(const Chunk& chunk, const glm::vec3& playerPos) const;
    bool ShouldUnloadChunk(const Chunk& chunk, const glm::vec3& playerPos) const;
    
    void EnableStreaming(bool enable) { m_streamingEnabled = enable; }
    bool IsStreamingEnabled() const { return m_streamingEnabled; }
    
private:
    ICore* m_core;
    ILogger* m_logger;
    
    glm::vec3 m_playerPosition{0.0f};
    float m_loadRadius = 200.0f;
    float m_unloadRadius = 300.0f;
    float m_chunkSize = 100.0f;
    bool m_streamingEnabled = true;
    
    float CalculateDistance(const glm::vec3& pos1, const glm::vec3& pos2) const;
};

// ============================================================================
// Instance Manager (GPU-Optimized)
// ============================================================================
class InstanceManager {
public:
    InstanceManager(ICore* core);
    ~InstanceManager();
    
    // Instance lifecycle
    uint32_t CreateInstance(const MeshInstance& instance);
    void UpdateInstance(uint32_t instanceId, const Transform& transform);
    void UpdateInstanceColor(uint32_t instanceId, const glm::vec4& color);
    void DestroyInstance(uint32_t instanceId);
    
    // Batch operations for performance
    void CreateInstances(const std::vector<MeshInstance>& instances);
    void UpdateInstances(const std::vector<std::pair<uint32_t, Transform>>& updates);
    void DestroyInstances(const std::vector<uint32_t>& instanceIds);
    
    // LOD management
    void UpdateLODs(const glm::vec3& cameraPosition);
    void SetLODBias(float bias) { m_lodBias = bias; }
    
    // Culling
    void UpdateCulling(const glm::vec3& cameraPos, const glm::vec3& cameraForward, float fov);
    void SetCullingEnabled(bool enabled) { m_cullingEnabled = enabled; }
    
    // Rendering
    void SubmitDrawCalls();
    void FlushUpdates();
    
    // Statistics
    uint32_t GetInstanceCount() const { return m_instanceCount; }
    uint32_t GetVisibleCount() const { return m_visibleCount; }
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    
    struct InstanceData {
        MeshInstance instance;
        bool visible = true;
        bool dirty = true;
        uint32_t lodLevel = 0;
    };
    
    std::unordered_map<uint32_t, InstanceData> m_instances;
    uint32_t m_nextInstanceId = 1;
    std::atomic<uint32_t> m_instanceCount{0};
    std::atomic<uint32_t> m_visibleCount{0};
    
    float m_lodBias = 1.0f;
    bool m_cullingEnabled = true;
    
    std::mutex m_instanceMutex;
    
    uint32_t CalculateLODLevel(const MeshInstance& instance, const glm::vec3& cameraPos) const;
    bool IsInstanceVisible(const MeshInstance& instance, const glm::vec3& cameraPos, const glm::vec3& cameraForward, float fov) const;
};

// ============================================================================
// Audio Manager
// ============================================================================
class AudioManager {
public:
    AudioManager(ICore* core);
    ~AudioManager();
    
    void LoadAudioZones(const std::vector<AudioZone>& zones);
    void UpdateAudioZones(const glm::vec3& listenerPosition);
    
    void PlayAmbientTrack(const std::string& track, float volume = 1.0f);
    void PlayCombatMusic(const std::string& track, float intensity = 1.0f);
    void StopAmbientTrack();
    void StopCombatMusic();
    
    void SetMasterVolume(float volume);
    void SetSFXVolume(float volume);
    void SetMusicVolume(float volume);
    
private:
    ICore* m_core;
    ILogger* m_logger;
    
    std::vector<AudioZone> m_audioZones;
    std::string m_currentAmbientTrack;
    std::string m_currentCombatTrack;
    
    float m_masterVolume = 1.0f;
    float m_sfxVolume = 0.8f;
    float m_musicVolume = 0.6f;
};

// ============================================================================
// Weather System
// ============================================================================
class WeatherSystem {
public:
    WeatherSystem(ICore* core);
    ~WeatherSystem();
    
    void LoadWeatherPatterns(const std::vector<WeatherPattern>& patterns);
    void UpdateWeather(float deltaTime);
    
    void SetWeather(const std::string& weatherName, float transitionTime = 30.0f);
    const WeatherPattern* GetCurrentWeather() const { return m_currentWeather; }
    
    void EnableDynamicWeather(bool enable) { m_dynamicWeatherEnabled = enable; }
    bool IsDynamicWeatherEnabled() const { return m_dynamicWeatherEnabled; }
    
private:
    ICore* m_core;
    ILogger* m_logger;
    
    std::vector<WeatherPattern> m_weatherPatterns;
    const WeatherPattern* m_currentWeather = nullptr;
    const WeatherPattern* m_targetWeather = nullptr;
    
    float m_transitionTime = 0.0f;
    float m_transitionDuration = 30.0f;
    float m_weatherTimer = 0.0f;
    
    bool m_dynamicWeatherEnabled = true;
    
    void TransitionToWeather(const WeatherPattern* weather);
    const WeatherPattern* SelectRandomWeather() const;
};

// ============================================================================
// Player Manager
// ============================================================================
class PlayerManager {
public:
    PlayerManager(ICore* core);
    ~PlayerManager();
    
    uint32_t SpawnPlayer(const Player& playerData, const glm::vec3& spawnPosition);
    void DespawnPlayer(uint32_t playerId);
    
    Player* GetPlayer(uint32_t playerId);
    const Player* GetPlayer(uint32_t playerId) const;
    
    void UpdatePlayer(uint32_t playerId, const Player& playerData);
    void UpdatePlayerTransform(uint32_t playerId, const Transform& transform);
    void UpdatePlayerStats(uint32_t playerId, const PlayerStats& stats);
    
    std::vector<uint32_t> GetAllPlayerIds() const;
    uint32_t GetPlayerCount() const { return static_cast<uint32_t>(m_players.size()); }
    
    // Spawn point management
    void LoadSpawnPoints(const std::vector<SpawnPoint>& spawnPoints);
    glm::vec3 GetSpawnPosition(int32_t team = -1) const;
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    
    std::unordered_map<uint32_t, Player> m_players;
    std::vector<SpawnPoint> m_spawnPoints;
    uint32_t m_nextPlayerId = 1;
    
    mutable std::mutex m_playerMutex;
};

// ============================================================================
// Main Level Manager
// ============================================================================
class V73LevelManager {
public:
    V73LevelManager(ICore* core);
    ~V73LevelManager();
    
    // Level lifecycle
    bool LoadLevel(const std::string& levelPath);
    bool UnloadLevel();
    bool ReloadLevel();
    
    // Chunk management
    bool LoadChunk(const std::string& chunkId);
    bool UnloadChunk(const std::string& chunkId);
    void UpdateChunkStreaming(float deltaTime);
    
    // Player management
    uint32_t SpawnPlayer(const Player& playerData, const glm::vec3& position = glm::vec3(0));
    void DespawnPlayer(uint32_t playerId);
    void UpdatePlayerPosition(uint32_t playerId, const glm::vec3& position);
    
    // System updates
    void Update(float deltaTime);
    void Render();
    
    // Event system
    void RegisterEventCallback(LevelEventCallback callback);
    void UnregisterEventCallback(LevelEventCallback callback);
    
    // Queries
    Level* GetCurrentLevel() { return m_currentLevel.get(); }
    const Level* GetCurrentLevel() const { return m_currentLevel.get(); }
    LevelState GetLevelState() const { return m_levelState; }
    
    Chunk* GetChunk(const std::string& chunkId);
    const Chunk* GetChunk(const std::string& chunkId) const;
    ChunkState GetChunkState(const std::string& chunkId) const;
    
    Player* GetPlayer(uint32_t playerId);
    const Player* GetPlayer(uint32_t playerId) const;
    
    // Performance
    const PerformanceMetrics& GetMetrics() const { return m_metrics; }
    void ResetMetrics() { m_metrics.Reset(); }
    
    // Configuration
    void SetStreamingEnabled(bool enabled);
    void SetStreamingRadius(float loadRadius, float unloadRadius);
    void SetLODBias(float bias);
    void SetCullingEnabled(bool enabled);
    
    // Debug
    void EnableDebugMode(bool enabled) { m_debugMode = enabled; }
    bool IsDebugModeEnabled() const { return m_debugMode; }
    void PrintLevelInfo() const;
    void PrintPerformanceStats() const;
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    IAssetProvider* m_assetProvider;
    
    // Current level data
    std::unique_ptr<Level> m_currentLevel;
    LevelState m_levelState = LevelState::Unloaded;
    std::unordered_map<std::string, ChunkState> m_chunkStates;
    
    // Subsystems
    std::unique_ptr<StreamingManager> m_streamingManager;
    std::unique_ptr<InstanceManager> m_instanceManager;
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<WeatherSystem> m_weatherSystem;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<TriggerSystem> m_triggerSystem;
    
    // Event system
    std::vector<LevelEventCallback> m_eventCallbacks;
    std::mutex m_eventMutex;
    
    // Performance tracking
    PerformanceMetrics m_metrics;
    
    // Configuration
    bool m_debugMode = false;
    
    // Threading
    std::thread m_streamingThread;
    std::atomic<bool> m_shouldStopStreaming{false};
    std::mutex m_levelMutex;
    
    // Internal methods
    bool LoadLevelFromJSON(const std::string& jsonText);
    bool LoadChunkFromJSON(const std::string& chunkId, const std::string& jsonText);
    
    void InitializeLevel();
    void CleanupLevel();
    
    void UpdateTimeOfDay(float deltaTime);
    void UpdateDynamicEvents(float deltaTime);
    void UpdatePerformanceMetrics();
    
    void FireEvent(const LevelEvent& event);
    
    // JSON parsing
    Level ParseLevelJSON(const nlohmann::json& json);
    Chunk ParseChunkJSON(const nlohmann::json& json);
    Player ParsePlayerJSON(const nlohmann::json& json);
    Transform ParseTransformJSON(const nlohmann::json& json);
    
    // Streaming thread function
    void StreamingThreadFunction();
};

} // namespace SecretEngine::Levels::V73