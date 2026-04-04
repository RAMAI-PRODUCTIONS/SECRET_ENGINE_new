// SecretEngine - v7.3 Ultra Full Level System Plugin Implementation

#include "V73LevelSystemPlugin.h"
#include "Streaming/NetworkedLevelStreamer.h"
#include "SceneLoader.h" // NEW: Scene loader
#include <SecretEngine/Components.h>
#include <SecretEngine/Scene.h> // NEW: Scene support
#include <SecretEngine/IRenderer.h>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// Plugin Interface Implementation
// ============================================================================
void V73LevelSystemPlugin::OnLoad(ICore* core) {
    m_core = core;
    m_world = core->GetWorld();
    m_logger = core->GetLogger();
    
    m_logger->LogInfo("V73LevelSystemPlugin", "Loading v7.3 Ultra Full Level System...");
    
    // Create level manager
    m_levelManager = std::make_unique<V73LevelManager>(core);
    
    // Register capability
    core->RegisterCapability("v73_level_system", this);
    
    m_logger->LogInfo("V73LevelSystemPlugin", "v7.3 Level System loaded successfully");
}

void V73LevelSystemPlugin::OnActivate() {
    m_logger->LogInfo("V73LevelSystemPlugin", "Activating v7.3 Level System...");
    
    // Disable streaming and networking for now - not needed for basic level loading
    m_streamingEnabled = false;
    m_networkingEnabled = false;
    
    // Setup event callbacks
    SetupEventCallbacks();
    
    m_logger->LogInfo("V73LevelSystemPlugin", "v7.3 Level System activated successfully");
    
    // NEW: Load chunk_0_full.json directly into the new Scene system
    m_logger->LogInfo("V73LevelSystemPlugin", "Loading chunk_0_full.json into new ECS Scene...");
    
    auto* scene = m_core->GetScene();
    if (scene) {
        // Create scene loader
        auto sceneLoader = std::make_unique<Levels::SceneLoader>(m_logger);
        
        // Load the chunk directly
        bool loaded = sceneLoader->LoadChunkIntoScene(scene, "levels/chunks/chunk_0_full.json");
        
        if (loaded) {
            m_logger->LogInfo("V73LevelSystemPlugin", "chunk_0_full.json loaded into Scene successfully!");
            
            // Start the scene
            scene->Play();
            m_logger->LogInfo("V73LevelSystemPlugin", "Scene is now playing!");
            
            // Log scene stats
            const auto& fastAccess = scene->GetFastAccess();
            char msg[512];
            snprintf(msg, sizeof(msg), "Scene stats: %zu entities, %zu meshes, %zu cameras, %zu lights",
                     scene->GetAllEntities().size(),
                     fastAccess.meshes.size(),
                     fastAccess.cameras.size(),
                     fastAccess.lights.size());
            m_logger->LogInfo("V73LevelSystemPlugin", msg);
        } else {
            m_logger->LogError("V73LevelSystemPlugin", "Failed to load chunk_0_full.json");
        }
    } else {
        m_logger->LogError("V73LevelSystemPlugin", "Scene not available from Core!");
    }
}

void V73LevelSystemPlugin::OnDeactivate() {
    m_logger->LogInfo("V73LevelSystemPlugin", "Deactivating v7.3 Level System...");
    
    // Streaming and networking disabled
    
    // Unload current level
    if (m_levelManager) {
        m_levelManager->UnloadLevel();
    }
    
    m_logger->LogInfo("V73LevelSystemPlugin", "v7.3 Level System deactivated");
}

void V73LevelSystemPlugin::OnUnload() {
    m_levelManager.reset();
    
    m_core = nullptr;
    m_world = nullptr;
    m_logger = nullptr;
    
    // Note: Logger might be invalid here, so no logging
}

void V73LevelSystemPlugin::OnUpdate(float dt) {
    if (!m_levelManager) return;
    
    // Update level manager
    m_levelManager->Update(dt);
    
    // Streaming and networking disabled for now
    
    // Update performance metrics
    UpdatePerformanceMetrics(dt);
}

void* V73LevelSystemPlugin::GetInterface(uint32_t interfaceId) {
    switch (interfaceId) {
        case 0: return m_levelManager.get();
        case 1: return &Streaming::GLevelStreaming;
        case 2: return &Streaming::GNetworkedLevelStreamer;
        default: return nullptr;
    }
}

// ============================================================================
// Configuration Methods
// ============================================================================
void V73LevelSystemPlugin::SetNetworkMode(bool bIsServer, uint16_t port, const std::string& serverAddress) {
    m_isServer = bIsServer;
    m_networkPort = port;
    m_serverAddress = serverAddress;
    m_networkingEnabled = true;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Network mode set: %s, Port: %d, Address: %s", 
             bIsServer ? "Server" : "Client", port, serverAddress.c_str());
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
}

// ============================================================================
// Level Operations
// ============================================================================
bool V73LevelSystemPlugin::LoadLevel(const std::string& levelPath) {
    if (!m_levelManager) {
        m_logger->LogError("V73LevelSystemPlugin", "Level manager not initialized");
        return false;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Loading level: %s", levelPath.c_str());
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
    
    // Clear all renderer instances before loading new level
    if (m_core) {
        auto* renderer = reinterpret_cast<IRenderer*>(m_core->GetCapability("rendering"));
        if (renderer) {
            m_logger->LogInfo("V73LevelSystemPlugin", "Clearing all renderer instances before loading new level...");
            renderer->ClearAllInstances();
        }
    }
    
    bool success = m_levelManager->LoadLevel(levelPath);
    
    if (success) {
        // Integrate with streaming system
        if (m_streamingEnabled) {
            const Level* currentLevel = m_levelManager->GetCurrentLevel();
            if (currentLevel) {
                Streaming::GLevelStreaming.LoadV73Level(*currentLevel);
                Streaming::GLevelStreaming.IntegrateWithV73Manager(m_levelManager.get());
            }
        }
        
        // Fire callback
        if (m_levelLoadedCallback) {
            const Level* level = m_levelManager->GetCurrentLevel();
            if (level) {
                m_levelLoadedCallback(level->level_id);
            }
        }
        
        snprintf(msg, sizeof(msg), "Successfully loaded level: %s", levelPath.c_str());
        m_logger->LogInfo("V73LevelSystemPlugin", msg);
    } else {
        snprintf(msg, sizeof(msg), "Failed to load level: %s", levelPath.c_str());
        m_logger->LogError("V73LevelSystemPlugin", msg);
    }
    
    return success;
}

bool V73LevelSystemPlugin::UnloadLevel() {
    if (!m_levelManager) return false;
    
    m_logger->LogInfo("V73LevelSystemPlugin", "Unloading current level...");
    
    // Clear all renderer instances before unloading level
    if (m_core) {
        auto* renderer = reinterpret_cast<IRenderer*>(m_core->GetCapability("rendering"));
        if (renderer) {
            m_logger->LogInfo("V73LevelSystemPlugin", "Clearing all renderer instances...");
            renderer->ClearAllInstances();
        }
    }
    
    bool success = m_levelManager->UnloadLevel();
    
    if (success) {
        // Clear player positions
        m_playerPositions.clear();
        
        m_logger->LogInfo("V73LevelSystemPlugin", "Level unloaded successfully");
    } else {
        m_logger->LogError("V73LevelSystemPlugin", "Failed to unload level");
    }
    
    return success;
}

uint32_t V73LevelSystemPlugin::SpawnPlayer(const Player& playerData, const glm::vec3& position) {
    if (!m_levelManager) return 0;
    
    uint32_t playerId = m_levelManager->SpawnPlayer(playerData, position);
    
    if (playerId != 0) {
        // Track player position
        m_playerPositions[playerId] = position;
        
        // Networking disabled
        
        // Fire callback
        if (m_playerSpawnedCallback) {
            m_playerSpawnedCallback(playerId, position);
        }
        
        char msg[256];
        snprintf(msg, sizeof(msg), "Spawned player %u (%s) at (%.1f, %.1f, %.1f)", 
                 playerId, playerData.username.c_str(), position.x, position.y, position.z);
        m_logger->LogInfo("V73LevelSystemPlugin", msg);
    }
    
    return playerId;
}

void V73LevelSystemPlugin::DespawnPlayer(uint32_t playerId) {
    if (!m_levelManager) return;
    
    // Note: DespawnPlayer not implemented in V73LevelManager yet
    // m_levelManager->DespawnPlayer(playerId);
    
    // Remove from tracking
    m_playerPositions.erase(playerId);
    
    // Networking disabled
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Despawned player %u", playerId);
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
}

void V73LevelSystemPlugin::UpdatePlayerPosition(uint32_t playerId, const glm::vec3& position) {
    if (!m_levelManager) return;
    
    // Note: UpdatePlayerPosition not implemented in V73LevelManager yet
    // m_levelManager->UpdatePlayerPosition(playerId, position);
    m_playerPositions[playerId] = position;
    
    // Networking disabled
}

// ============================================================================
// Statistics
// ============================================================================
V73LevelSystemPlugin::SystemStats V73LevelSystemPlugin::GetSystemStats() const {
    SystemStats stats;
    
    if (m_levelManager) {
        const auto& metrics = m_levelManager->GetMetrics();
        stats.visible_instances = metrics.visible_instances;
        stats.memory_usage_mb = metrics.total_memory_usage / (1024.0f * 1024.0f);
    }
    
    if (m_streamingEnabled) {
        auto loadedChunks = Streaming::GLevelStreaming.GetLoadedChunks();
        stats.loaded_chunks = static_cast<uint32_t>(loadedChunks.size());
    }
    
    if (m_networkingEnabled) {
        const auto& networkStats = Streaming::GNetworkedLevelStreamer.GetNetworkStats();
        stats.network_bandwidth_kbps = networkStats.Bandwidth;
        stats.network_clients = networkStats.ConnectedClients;
    }
    
    stats.active_players = static_cast<uint32_t>(m_playerPositions.size());
    stats.frame_time_ms = m_averageFrameTime;
    
    return stats;
}

void V73LevelSystemPlugin::PrintSystemInfo() const {
    if (!m_logger) return;
    
    SystemStats stats = GetSystemStats();
    
    char msg[1024];
    snprintf(msg, sizeof(msg),
             "=== v7.3 Level System Statistics ===\n"
             "Loaded Chunks: %u\n"
             "Active Players: %u\n"
             "Visible Instances: %u\n"
             "Memory Usage: %.2f MB\n"
             "Network Bandwidth: %.2f KB/s\n"
             "Network Clients: %u\n"
             "Average Frame Time: %.2f ms\n"
             "Streaming Enabled: %s\n"
             "Networking Enabled: %s\n"
             "Server Mode: %s",
             stats.loaded_chunks,
             stats.active_players,
             stats.visible_instances,
             stats.memory_usage_mb,
             stats.network_bandwidth_kbps,
             stats.network_clients,
             stats.frame_time_ms,
             m_streamingEnabled ? "Yes" : "No",
             m_networkingEnabled ? "Yes" : "No",
             m_isServer ? "Yes" : "No");
    
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
}

// ============================================================================
// Private Methods
// ============================================================================
void V73LevelSystemPlugin::InitializeStreamingSystem() {
    m_logger->LogInfo("V73LevelSystemPlugin", "Initializing streaming system...");
    
    // Streaming system will be initialized when a level is loaded
    // This is because we need level data to configure streaming
    
    m_logger->LogInfo("V73LevelSystemPlugin", "Streaming system ready");
}

void V73LevelSystemPlugin::InitializeNetworkSystem() {
    m_logger->LogInfo("V73LevelSystemPlugin", "Initializing network system...");
    
    if (m_isServer) {
        Streaming::GNetworkedLevelStreamer.InitializeAsServer(m_networkPort);
    } else {
        Streaming::GNetworkedLevelStreamer.InitializeAsClient(m_serverAddress, m_networkPort);
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Network system initialized as %s on port %d", 
             m_isServer ? "server" : "client", m_networkPort);
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
}

void V73LevelSystemPlugin::SetupEventCallbacks() {
    if (!m_levelManager) return;
    
    // Setup level manager callbacks
    m_levelManager->RegisterEventCallback([this](const LevelEvent& event) {
        OnLevelLoaded(event);
    });
    
    // Setup streaming callbacks
    if (m_streamingEnabled) {
        Streaming::GLevelStreaming.SetOnChunkLoaded([this](const std::string& chunkId) {
            OnChunkLoaded(chunkId);
        });
    }
    
    // Setup network callbacks
    if (m_networkingEnabled) {
        Streaming::GNetworkedLevelStreamer.SetOnClientReady([this](uint32_t clientId) {
            OnNetworkClientConnected(clientId);
        });
        
        Streaming::GNetworkedLevelStreamer.SetOnClientDisconnected([this](uint32_t clientId) {
            OnNetworkClientDisconnected(clientId);
        });
    }
}

void V73LevelSystemPlugin::UpdatePlayerStreaming(float deltaTime) {
    if (m_playerPositions.empty()) return;
    
    // Update streaming system with player positions
    Streaming::GLevelStreaming.UpdatePlayerPositions(m_playerPositions);
}

void V73LevelSystemPlugin::UpdateNetworking(float deltaTime) {
    // Network updates are handled in the main OnUpdate call
}

void V73LevelSystemPlugin::UpdatePerformanceMetrics(float deltaTime) {
    m_frameTimeAccumulator += deltaTime * 1000.0f; // Convert to ms
    m_frameCount++;
    
    // Update average every second
    if (m_frameTimeAccumulator >= 1000.0f) {
        m_averageFrameTime = m_frameTimeAccumulator / m_frameCount;
        m_frameTimeAccumulator = 0.0f;
        m_frameCount = 0;
    }
}

// ============================================================================
// Event Handlers
// ============================================================================
void V73LevelSystemPlugin::OnLevelLoaded(const LevelEvent& event) {
    if (event.type == LevelEvent::LevelLoaded) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Level loaded event: %s", event.level_id.c_str());
        m_logger->LogInfo("V73LevelSystemPlugin", msg);
    }
}

void V73LevelSystemPlugin::OnChunkLoaded(const std::string& chunkId) {
    if (m_chunkLoadedCallback) {
        m_chunkLoadedCallback(chunkId);
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Chunk loaded: %s", chunkId.c_str());
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
}

void V73LevelSystemPlugin::OnPlayerPositionChanged(uint32_t playerId, const glm::vec3& position) {
    UpdatePlayerPosition(playerId, position);
}

void V73LevelSystemPlugin::OnNetworkClientConnected(uint32_t clientId) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Network client connected: %u", clientId);
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
}

void V73LevelSystemPlugin::OnNetworkClientDisconnected(uint32_t clientId) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Network client disconnected: %u", clientId);
    m_logger->LogInfo("V73LevelSystemPlugin", msg);
    
    // Clean up player data
    DespawnPlayer(clientId);
}

} // namespace SecretEngine::Levels::V73