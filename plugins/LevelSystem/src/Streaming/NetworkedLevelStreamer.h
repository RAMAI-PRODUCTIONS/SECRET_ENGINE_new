// SecretEngine - Networked Level Streamer for Multiplayer Sync

#pragma once
#include "LevelStreamingSubsystem.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

namespace SecretEngine::Levels::Streaming {

// ============================================================================
// Client Streaming State
// ============================================================================
struct FClientStreamingState {
    uint32_t ClientId;
    std::unordered_map<std::string, ELevelState> ChunkStates;
    std::unordered_map<std::string, float> ChunkProgress;
    uint64_t LastUpdateFrame = 0;
    bool bIsReady = false;
    glm::vec3 LastKnownPosition{0.0f};
    float ConnectionQuality = 1.0f;
    uint32_t PacketsSent = 0;
    uint32_t PacketsReceived = 0;
};

// ============================================================================
// Network Message Types
// ============================================================================
enum class ENetworkMessageType : uint8_t {
    PlayerPositionUpdate = 0,
    ChunkStateUpdate,
    ChunkDataReplication,
    ChunkUnloadNotification,
    ClientReadyNotification,
    ServerFullState,
    StreamingConfigUpdate
};

struct FNetworkMessage {
    ENetworkMessageType Type;
    uint32_t ClientId;
    uint32_t DataSize;
    std::vector<uint8_t> Data;
    uint64_t Timestamp;
    
    void Serialize(std::vector<uint8_t>& OutData) const;
    void Deserialize(const std::vector<uint8_t>& InData);
};

// ============================================================================
// Networked Level Streamer
// ============================================================================
class FNetworkedLevelStreamer {
public:
    static FNetworkedLevelStreamer& Get();
    
    // Initialization
    void InitializeAsServer(uint16_t Port);
    void InitializeAsClient(const std::string& ServerAddress, uint16_t Port);
    void Shutdown();
    
    // Server methods
    void RegisterClient(uint32_t ClientId, const FPlayerStreamingData& InitialData);
    void UnregisterClient(uint32_t ClientId);
    void UpdateClientStreaming(uint32_t ClientId, const std::vector<std::string>& VisibleChunks);
    void SendFullStateToClient(uint32_t ClientId);
    void BroadcastChunkState(const std::string& ChunkId, ELevelState State, float Progress);
    
    // Client methods
    void SendPositionUpdate(const glm::vec3& Position);
    void ProcessServerState(const std::vector<uint8_t>& StateData);
    void RequestChunkLoad(const std::string& ChunkId, uint32_t Priority);
    
    // Replication
    void ReplicateChunkToClients(const std::string& ChunkId, const std::vector<uint8_t>& ChunkData);
    void ReplicateChunkUnload(const std::string& ChunkId);
    void ReplicateToSpecificClients(const std::string& ChunkId, const std::vector<uint32_t>& ClientIds, const std::vector<uint8_t>& ChunkData);
    
    // Tick
    void Tick(float DeltaTime);
    
    // Network state
    bool IsServer() const { return bIsServer; }
    bool IsClient() const { return bIsClient; }
    bool IsConnected() const { return bIsConnected; }
    
    // Statistics
    struct FNetworkStats {
        uint32_t TotalBytesSent = 0;
        uint32_t TotalBytesReceived = 0;
        uint32_t PacketsSent = 0;
        uint32_t PacketsReceived = 0;
        uint32_t PacketsLost = 0;
        float AverageLatency = 0.0f;
        float Bandwidth = 0.0f; // KB/s
        uint32_t ConnectedClients = 0;
    };
    
    const FNetworkStats& GetNetworkStats() const { return NetworkStats; }
    void ResetNetworkStats() { NetworkStats = FNetworkStats{}; }
    
    // Events
    using FOnClientReady = std::function<void(uint32_t ClientId)>;
    using FOnClientDisconnected = std::function<void(uint32_t ClientId)>;
    using FOnChunkReceived = std::function<void(const std::string& ChunkId, const std::vector<uint8_t>& Data)>;
    using FOnChunkUnloadReceived = std::function<void(const std::string& ChunkId)>;
    using FOnServerStateReceived = std::function<void(const std::vector<uint8_t>& StateData)>;
    
    void SetOnClientReady(FOnClientReady Callback) { OnClientReady = Callback; }
    void SetOnClientDisconnected(FOnClientDisconnected Callback) { OnClientDisconnected = Callback; }
    void SetOnChunkReceived(FOnChunkReceived Callback) { OnChunkReceived = Callback; }
    void SetOnChunkUnloadReceived(FOnChunkUnloadReceived Callback) { OnChunkUnloadReceived = Callback; }
    void SetOnServerStateReceived(FOnServerStateReceived Callback) { OnServerStateReceived = Callback; }
    
    // Configuration
    void SetCompressionEnabled(bool bEnabled) { bCompressionEnabled = bEnabled; }
    void SetMaxBandwidthKBps(uint32_t MaxKBps) { MaxBandwidthKBps = MaxKBps; }
    void SetClientTimeout(float TimeoutSeconds) { ClientTimeoutSeconds = TimeoutSeconds; }
    void SetReplicationRate(float Rate) { ReplicationRate = Rate; }
    
private:
    FNetworkedLevelStreamer() = default;
    ~FNetworkedLevelStreamer() = default;
    
    void ProcessServerTick(float DeltaTime);
    void ProcessClientTick(float DeltaTime);
    void SendReplicationData();
    void ReceiveReplicationData();
    void ProcessIncomingMessage(const FNetworkMessage& Message);
    void SendMessage(const FNetworkMessage& Message, uint32_t TargetClientId = 0);
    void BroadcastMessage(const FNetworkMessage& Message);
    
    // Bandwidth management
    void UpdateBandwidthUsage(float DeltaTime);
    bool CanSendData(uint32_t DataSize) const;
    void QueueMessage(const FNetworkMessage& Message, uint32_t TargetClientId = 0);
    void ProcessMessageQueue();
    
    // Compression
    std::vector<uint8_t> CompressData(const std::vector<uint8_t>& Data) const;
    std::vector<uint8_t> DecompressData(const std::vector<uint8_t>& CompressedData) const;
    
    // Client management
    void UpdateClientTimeouts(float DeltaTime);
    void RemoveTimedOutClients();
    FClientStreamingState* GetClientState(uint32_t ClientId);
    
    // Server state
    bool bIsServer = false;
    bool bIsClient = false;
    bool bIsConnected = false;
    uint16_t ServerPort = 0;
    std::string ServerAddress;
    
    std::unordered_map<uint32_t, FClientStreamingState> Clients;
    std::unordered_map<std::string, std::vector<uint8_t>> ReplicatedChunkData;
    uint32_t NextClientId = 1;
    
    // Client state
    uint32_t LocalClientId = 0;
    glm::vec3 LastSentPosition{0.0f};
    float PositionUpdateTimer = 0.0f;
    std::unordered_map<std::string, ELevelState> ServerChunkStates;
    
    // Network configuration
    bool bCompressionEnabled = true;
    uint32_t MaxBandwidthKBps = 1024; // 1 MB/s
    float ClientTimeoutSeconds = 30.0f;
    float ReplicationRate = 10.0f; // 10 Hz
    float ReplicationTimer = 0.0f;
    
    // Message queue for bandwidth management
    struct FQueuedMessage {
        FNetworkMessage Message;
        uint32_t TargetClientId;
        float Priority;
        uint64_t QueueTime;
    };
    
    std::vector<FQueuedMessage> MessageQueue;
    std::mutex MessageQueueMutex;
    
    // Network statistics
    FNetworkStats NetworkStats;
    float StatsUpdateTimer = 0.0f;
    uint32_t BytesSentThisSecond = 0;
    uint32_t BytesReceivedThisSecond = 0;
    
    // Network socket (placeholder for actual implementation)
    struct FSocketHandle {
        bool IsValid() const { return true; }
        void Send(const std::vector<uint8_t>& Data, uint32_t ClientId = 0) const {}
        std::vector<uint8_t> Receive() const { return {}; }
        void Close() {}
    } NetworkSocket;
    
    // Threading
    std::thread NetworkThread;
    std::atomic<bool> bShouldStopNetworking{false};
    std::mutex ClientsMutex;
    std::mutex ReplicationMutex;
    
    // Events
    FOnClientReady OnClientReady;
    FOnClientDisconnected OnClientDisconnected;
    FOnChunkReceived OnChunkReceived;
    FOnChunkUnloadReceived OnChunkUnloadReceived;
    FOnServerStateReceived OnServerStateReceived;
    
    // Network thread function
    void NetworkThreadFunction();
};

// Singleton access
inline FNetworkedLevelStreamer& GNetworkedLevelStreamer = FNetworkedLevelStreamer::Get();

// ============================================================================
// Network Integration Helper
// ============================================================================
class FLevelStreamingNetworkIntegration {
public:
    static void InitializeForGameMode(bool bIsServer, uint16_t Port = 7777, const std::string& ServerAddress = "127.0.0.1");
    static void SetupCallbacks();
    static void UpdatePlayerPositions(const std::unordered_map<uint32_t, glm::vec3>& PlayerPositions);
    static void OnPlayerJoined(uint32_t PlayerId, const glm::vec3& SpawnPosition);
    static void OnPlayerLeft(uint32_t PlayerId);
    static void Shutdown();
    
private:
    static bool bIsInitialized;
    static std::unordered_map<uint32_t, std::unique_ptr<FPlayerLevelManager>> PlayerManagers;
};

} // namespace SecretEngine::Levels::Streaming