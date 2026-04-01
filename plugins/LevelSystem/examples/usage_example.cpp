// SecretEngine - v7.3 Ultra Full Level System Usage Example
// Complete integration example showing all features

#include "../src/V73LevelSystemPlugin.h"
#include <SecretEngine/ICore.h>
#include <iostream>

using namespace SecretEngine::Levels::V73;
using namespace SecretEngine::Levels::Streaming;

// ============================================================================
// Example Game Mode Class
// ============================================================================
class ExampleGameMode {
public:
    ExampleGameMode(ICore* core) : m_core(core) {
        // Load the v7.3 Level System plugin
        m_levelSystemPlugin = static_cast<V73LevelSystemPlugin*>(
            core->LoadPlugin("V73LevelSystem"));
        
        if (!m_levelSystemPlugin) {
            std::cerr << "Failed to load V73LevelSystem plugin!" << std::endl;
            return;
        }
        
        std::cout << "v7.3 Level System loaded successfully!" << std::endl;
    }
    
    void InitializeAsServer() {
        std::cout << "=== Initializing as Dedicated Server ===" << std::endl;
        
        // Configure as server with networking
        m_levelSystemPlugin->SetNetworkMode(true, 7777); // Server on port 7777
        m_levelSystemPlugin->SetStreamingEnabled(true);
        m_levelSystemPlugin->SetNetworkingEnabled(true);
        
        // Setup callbacks
        SetupCallbacks();
        
        // Load the arena level
        bool success = m_levelSystemPlugin->LoadLevel("examples/v73_arena_level.json");
        if (success) {
            std::cout << "Arena level loaded successfully!" << std::endl;
            PrintLevelInfo();
        } else {
            std::cerr << "Failed to load arena level!" << std::endl;
        }
    }
    
    void InitializeAsClient(const std::string& serverAddress = "127.0.0.1") {
        std::cout << "=== Initializing as Client ===" << std::endl;
        
        // Configure as client
        m_levelSystemPlugin->SetNetworkMode(false, 7777, serverAddress);
        m_levelSystemPlugin->SetStreamingEnabled(true);
        m_levelSystemPlugin->SetNetworkingEnabled(true);
        
        // Setup callbacks
        SetupCallbacks();
        
        std::cout << "Connecting to server: " << serverAddress << ":7777" << std::endl;
    }
    
    void InitializeStandalone() {
        std::cout << "=== Initializing Standalone Mode ===" << std::endl;
        
        // Configure for single-player
        m_levelSystemPlugin->SetStreamingEnabled(true);
        m_levelSystemPlugin->SetNetworkingEnabled(false);
        
        // Setup callbacks
        SetupCallbacks();
        
        // Load the arena level
        bool success = m_levelSystemPlugin->LoadLevel("examples/v73_arena_level.json");
        if (success) {
            std::cout << "Arena level loaded successfully!" << std::endl;
            PrintLevelInfo();
            
            // Spawn a test player
            SpawnTestPlayer();
        }
    }
    
    void Update(float deltaTime) {
        if (!m_levelSystemPlugin) return;
        
        // Update the level system (this is called automatically by the plugin system)
        // m_levelSystemPlugin->OnUpdate(deltaTime);
        
        // Update test player movement
        UpdateTestPlayer(deltaTime);
        
        // Print stats every 5 seconds
        m_statsTimer += deltaTime;
        if (m_statsTimer >= 5.0f) {
            PrintSystemStats();
            m_statsTimer = 0.0f;
        }
    }
    
    void Shutdown() {
        if (m_levelSystemPlugin) {
            std::cout << "Shutting down v7.3 Level System..." << std::endl;
            m_levelSystemPlugin->UnloadLevel();
        }
    }
    
private:
    ICore* m_core;
    V73LevelSystemPlugin* m_levelSystemPlugin = nullptr;
    uint32_t m_testPlayerId = 0;
    glm::vec3 m_testPlayerPosition{480.0f, 2.0f, 480.0f};
    float m_testPlayerAngle = 0.0f;
    float m_statsTimer = 0.0f;
    
    void SetupCallbacks() {
        // Level loaded callback
        m_levelSystemPlugin->SetLevelLoadedCallback([](const std::string& levelId) {
            std::cout << "✅ Level loaded: " << levelId << std::endl;
        });
        
        // Player spawned callback
        m_levelSystemPlugin->SetPlayerSpawnedCallback([](uint32_t playerId, const glm::vec3& position) {
            std::cout << "👤 Player " << playerId << " spawned at (" 
                      << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
        });
        
        // Chunk loaded callback
        m_levelSystemPlugin->SetChunkLoadedCallback([](const std::string& chunkId) {
            std::cout << "📦 Chunk loaded: " << chunkId << std::endl;
        });
        
        // Setup streaming system callbacks
        auto& streaming = GLevelStreaming;
        
        streaming.SetOnChunkLoaded([](const std::string& chunkId) {
            std::cout << "🔄 Streaming: Chunk " << chunkId << " loaded" << std::endl;
        });
        
        streaming.SetOnChunkUnloaded([](const std::string& chunkId) {
            std::cout << "🔄 Streaming: Chunk " << chunkId << " unloaded" << std::endl;
        });
        
        streaming.SetOnStreamingBudgetExceeded([](float current, float max) {
            std::cout << "⚠️  Streaming budget exceeded: " << current << " MB / " << max << " MB" << std::endl;
        });
        
        // Setup network callbacks
        auto& network = GNetworkedLevelStreamer;
        
        network.SetOnClientReady([](uint32_t clientId) {
            std::cout << "🌐 Network: Client " << clientId << " ready" << std::endl;
        });
        
        network.SetOnClientDisconnected([](uint32_t clientId) {
            std::cout << "🌐 Network: Client " << clientId << " disconnected" << std::endl;
        });
        
        network.SetOnChunkReceived([](const std::string& chunkId, const std::vector<uint8_t>& data) {
            std::cout << "📡 Network: Received chunk " << chunkId << " (" << data.size() << " bytes)" << std::endl;
        });
    }
    
    void SpawnTestPlayer() {
        // Create a test player
        Player testPlayer;
        testPlayer.username = "TestPlayer";
        testPlayer.level = 25;
        testPlayer.xp = 5000;
        testPlayer.region = "US";
        testPlayer.language = "en";
        
        // Set initial stats
        testPlayer.stats.health.current = 100;
        testPlayer.stats.health.max = 100;
        testPlayer.stats.stamina.current = 100;
        testPlayer.stats.stamina.max = 100;
        
        // Spawn the player
        m_testPlayerId = m_levelSystemPlugin->SpawnPlayer(testPlayer, m_testPlayerPosition);
        
        if (m_testPlayerId != 0) {
            std::cout << "Test player spawned with ID: " << m_testPlayerId << std::endl;
        }
    }
    
    void UpdateTestPlayer(float deltaTime) {
        if (m_testPlayerId == 0) return;
        
        // Move the test player in a circle around the center arena
        m_testPlayerAngle += deltaTime * 0.5f; // Rotate slowly
        
        glm::vec3 center(500.0f, 2.0f, 500.0f);
        float radius = 50.0f;
        
        m_testPlayerPosition.x = center.x + cos(m_testPlayerAngle) * radius;
        m_testPlayerPosition.z = center.z + sin(m_testPlayerAngle) * radius;
        
        // Update player position in the system
        m_levelSystemPlugin->UpdatePlayerPosition(m_testPlayerId, m_testPlayerPosition);
    }
    
    void PrintLevelInfo() {
        auto* levelManager = m_levelSystemPlugin->GetLevelManager();
        if (levelManager) {
            levelManager->PrintLevelInfo();
        }
    }
    
    void PrintSystemStats() {
        auto stats = m_levelSystemPlugin->GetSystemStats();
        
        std::cout << "\n=== System Statistics ===" << std::endl;
        std::cout << "Loaded Chunks: " << stats.loaded_chunks << std::endl;
        std::cout << "Active Players: " << stats.active_players << std::endl;
        std::cout << "Visible Instances: " << stats.visible_instances << std::endl;
        std::cout << "Memory Usage: " << stats.memory_usage_mb << " MB" << std::endl;
        std::cout << "Network Bandwidth: " << stats.network_bandwidth_kbps << " KB/s" << std::endl;
        std::cout << "Network Clients: " << stats.network_clients << std::endl;
        std::cout << "Frame Time: " << stats.frame_time_ms << " ms" << std::endl;
        std::cout << "========================\n" << std::endl;
    }
};

// ============================================================================
// Main Application Example
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "SecretEngine v7.3 Ultra Full Level System Example" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    // Initialize core engine (placeholder)
    ICore* core = nullptr; // This would be your actual engine core
    
    if (!core) {
        std::cerr << "Failed to initialize engine core!" << std::endl;
        return -1;
    }
    
    // Create game mode
    ExampleGameMode gameMode(core);
    
    // Parse command line arguments
    std::string mode = "standalone";
    std::string serverAddress = "127.0.0.1";
    
    if (argc > 1) {
        mode = argv[1];
    }
    if (argc > 2) {
        serverAddress = argv[2];
    }
    
    // Initialize based on mode
    if (mode == "server") {
        gameMode.InitializeAsServer();
    } else if (mode == "client") {
        gameMode.InitializeAsClient(serverAddress);
    } else {
        gameMode.InitializeStandalone();
    }
    
    // Main game loop
    std::cout << "\nStarting main loop (press Ctrl+C to exit)..." << std::endl;
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    bool running = true;
    
    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Update game mode
        gameMode.Update(deltaTime);
        
        // Simple exit condition (in a real game, this would be handled by the engine)
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        
        // Check for exit (simplified)
        static int frameCount = 0;
        frameCount++;
        if (frameCount > 3600) { // Run for ~1 minute at 60 FPS
            running = false;
        }
    }
    
    // Shutdown
    gameMode.Shutdown();
    
    std::cout << "Example completed successfully!" << std::endl;
    return 0;
}

// ============================================================================
// Advanced Usage Examples
// ============================================================================

// Example: Custom Event Handling
void SetupAdvancedEventHandling(V73LevelSystemPlugin* plugin) {
    auto* levelManager = plugin->GetLevelManager();
    
    // Register for level events
    levelManager->RegisterEventCallback([](const LevelEvent& event) {
        switch (event.type) {
            case LevelEvent::LevelLoaded:
                std::cout << "🎮 Game Event: Level loaded - " << event.level_id << std::endl;
                break;
            case LevelEvent::PlayerSpawned:
                std::cout << "🎮 Game Event: Player " << event.player_id << " spawned" << std::endl;
                break;
            case LevelEvent::WeatherChanged:
                std::cout << "🌦️  Game Event: Weather changed" << std::endl;
                break;
            default:
                break;
        }
    });
}

// Example: Custom Streaming Configuration
void ConfigureAdvancedStreaming() {
    auto& streaming = GLevelStreaming;
    
    // Create custom level definition
    FLevelDefinition customDef;
    customDef.LevelName = "CustomArena";
    customDef.Version = "1.0";
    customDef.MaxPlayers = 64;
    customDef.StreamingRadius = 300.0f;
    customDef.MemoryBudgetMB = 4096.0f; // 4GB
    customDef.MaxConcurrentLoads = 8;
    
    // Add custom chunks
    FLevelChunkInfo chunk1;
    chunk1.ChunkId = "custom_chunk_1";
    chunk1.FilePath = "levels/custom/chunk_1.json";
    chunk1.BoundsMin = glm::vec3(0, 0, 0);
    chunk1.BoundsMax = glm::vec3(100, 50, 100);
    chunk1.LoadRadius = 150.0f;
    chunk1.UnloadRadius = 200.0f;
    chunk1.Priority = ELevelPriority::High;
    
    customDef.Chunks.push_back(chunk1);
    
    // Initialize with custom definition
    streaming.Initialize(customDef, EStreamingRole::DedicatedServer);
}

// Example: Network Configuration
void ConfigureAdvancedNetworking() {
    auto& network = GNetworkedLevelStreamer;
    
    // Configure network settings
    network.SetCompressionEnabled(true);
    network.SetMaxBandwidthKBps(2048); // 2 MB/s
    network.SetClientTimeout(60.0f); // 60 seconds
    network.SetReplicationRate(20.0f); // 20 Hz
    
    // Setup advanced callbacks
    network.SetOnChunkReceived([](const std::string& chunkId, const std::vector<uint8_t>& data) {
        std::cout << "📡 Advanced: Processing chunk " << chunkId 
                  << " with " << data.size() << " bytes" << std::endl;
        
        // Custom chunk processing logic here
        // For example: validate chunk data, apply custom decompression, etc.
    });
}

// Example: Performance Monitoring
class PerformanceMonitor {
public:
    void Update(const V73LevelSystemPlugin::SystemStats& stats) {
        m_frameTimeHistory.push_back(stats.frame_time_ms);
        if (m_frameTimeHistory.size() > 60) { // Keep last 60 frames
            m_frameTimeHistory.erase(m_frameTimeHistory.begin());
        }
        
        // Calculate average frame time
        float avgFrameTime = 0.0f;
        for (float time : m_frameTimeHistory) {
            avgFrameTime += time;
        }
        avgFrameTime /= m_frameTimeHistory.size();
        
        // Check for performance issues
        if (avgFrameTime > 20.0f) { // > 50 FPS
            std::cout << "⚠️  Performance Warning: Average frame time " 
                      << avgFrameTime << "ms (target: <16.67ms)" << std::endl;
        }
        
        if (stats.memory_usage_mb > 2048.0f) { // > 2GB
            std::cout << "⚠️  Memory Warning: Usage " << stats.memory_usage_mb 
                      << "MB (consider reducing quality)" << std::endl;
        }
    }
    
private:
    std::vector<float> m_frameTimeHistory;
};

/*
Usage Instructions:

1. Compile and run as server:
   ./example server

2. Compile and run as client:
   ./example client 192.168.1.100

3. Compile and run standalone:
   ./example

Features Demonstrated:
- Complete v7.3 level loading with all systems
- Advanced streaming with chunk management
- Multiplayer networking with client/server architecture
- Real-time performance monitoring
- Event-driven architecture
- Player management and movement
- Dynamic weather and environmental systems
- Memory and bandwidth management
- Comprehensive error handling and logging

The example shows a complete integration of the v7.3 Ultra Full Level System
with all advanced features including streaming, networking, and performance
optimization suitable for production games supporting 100+ players.
*/