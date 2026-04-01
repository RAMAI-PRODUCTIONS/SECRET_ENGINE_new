// SecretEngine - v7.3 Ultra Full Level System Plugin
// Complete integration of v7.3 specification with advanced streaming

#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include "V73LevelManager.h"
#include "Streaming/LevelStreamingSubsystem.h"
#include "Streaming/NetworkedLevelStreamer.h"

namespace SecretEngine::Levels::V73 {

// ============================================================================
// v7.3 Level System Plugin
// ============================================================================
class V73LevelSystemPlugin : public IPlugin {
public:
    V73LevelSystemPlugin() = default;
    ~V73LevelSystemPlugin() = default;
    
    // IPlugin interface
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t interfaceId) override;
    const char* GetName() const override { return "V73LevelSystem"; }
    uint32_t GetVersion() const override { return 73; } // v7.3
    
    // Access to managers
    V73LevelManager* GetLevelManager() { return m_levelManager.get(); }
    Streaming::FLevelStreamingSubsystem* GetStreamingSubsystem() { return &Streaming::GLevelStreaming; }
    Streaming::FNetworkedLevelStreamer* GetNetworkStreamer() { return &Streaming::GNetworkedLevelStreamer; }
    
    // Configuration
    void SetNetworkMode(bool bIsServer, uint16_t port = 7777, const std::string& serverAddress = "127.0.0.1");
    void SetStreamingEnabled(bool enabled) { m_streamingEnabled = enabled; }
    void SetNetworkingEnabled(bool enabled) { m_networkingEnabled = enabled; }
    
    // Level operations
    bool LoadLevel(const std::string& levelPath);
    bool UnloadLevel();
    uint32_t SpawnPlayer(const Player& playerData, const glm::vec3& position = glm::vec3(0));
    void DespawnPlayer(uint32_t playerId);
    void UpdatePlayerPosition(uint32_t playerId, const glm::vec3& position);
    
    // Events
    using LevelLoadedCallback = std::function<void(const std::string& levelId)>;
    using PlayerSpawnedCallback = std::function<void(uint32_t playerId, const glm::vec3& position)>;
    using ChunkLoadedCallback = std::function<void(const std::string& chunkId)>;
    
    void SetLevelLoadedCallback(LevelLoadedCallback callback) { m_levelLoadedCallback = callback; }
    void SetPlayerSpawnedCallback(PlayerSpawnedCallback callback) { m_playerSpawnedCallback = callback; }
    void SetChunkLoadedCallback(ChunkLoadedCallback callback) { m_chunkLoadedCallback = callback; }
    
    // Statistics
    struct SystemStats {
        uint32_t loaded_chunks = 0;
        uint32_t active_players = 0;
        uint32_t visible_instances = 0;
        float memory_usage_mb = 0.0f;
        float network_bandwidth_kbps = 0.0f;
        uint32_t network_clients = 0;
        float frame_time_ms = 0.0f;
    };
    
    SystemStats GetSystemStats() const;
    void PrintSystemInfo() const;
    
private:
    // Core
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
    
    // Managers
    std::unique_ptr<V73LevelManager> m_levelManager;
    
    // Configuration
    bool m_streamingEnabled = true;
    bool m_networkingEnabled = false;
    bool m_isServer = false;
    uint16_t m_networkPort = 7777;
    std::string m_serverAddress = "127.0.0.1";
    
    // Player tracking
    std::unordered_map<uint32_t, glm::vec3> m_playerPositions;
    
    // Callbacks
    LevelLoadedCallback m_levelLoadedCallback;
    PlayerSpawnedCallback m_playerSpawnedCallback;
    ChunkLoadedCallback m_chunkLoadedCallback;
    
    // Internal methods
    void InitializeStreamingSystem();
    void InitializeNetworkSystem();
    void SetupEventCallbacks();
    void UpdatePlayerStreaming(float deltaTime);
    void UpdateNetworking(float deltaTime);
    void UpdatePerformanceMetrics(float deltaTime);
    
    // Event handlers
    void OnLevelLoaded(const LevelEvent& event);
    void OnChunkLoaded(const std::string& chunkId);
    void OnPlayerPositionChanged(uint32_t playerId, const glm::vec3& position);
    void OnNetworkClientConnected(uint32_t clientId);
    void OnNetworkClientDisconnected(uint32_t clientId);
    
    // Performance tracking
    float m_frameTimeAccumulator = 0.0f;
    uint32_t m_frameCount = 0;
    float m_averageFrameTime = 0.0f;
};

// ============================================================================
// Plugin Factory Function
// ============================================================================

// Platform-specific export macro
#ifdef _WIN32
    #define PLUGIN_EXPORT __declspec(dllexport)
#else
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    PLUGIN_EXPORT IPlugin* CreatePlugin();
    PLUGIN_EXPORT void DestroyPlugin(IPlugin* plugin);
    PLUGIN_EXPORT const char* GetPluginName();
    PLUGIN_EXPORT uint32_t GetPluginVersion();
}

} // namespace SecretEngine::Levels::V73