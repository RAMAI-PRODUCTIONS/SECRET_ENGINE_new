// SecretEngine - Advanced Level Streaming System with Multiplayer Sync
// Integrated with v7.3 Ultra Full Level System

#pragma once
#include "../V73LevelSystem.h"
#include <memory>
#include <unordered_map>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <glm/glm.hpp>

// Forward declarations
namespace SecretEngine::Levels::V73 {
    class V73LevelManager;
}

namespace SecretEngine::Levels::Streaming {

// ============================================================================
// Instance Types for Streaming
// ============================================================================
struct FMeshInstance {
    uint32_t EntityId = 0;
    uint32_t MeshId = 0;
    uint32_t MaterialId = 0;
    glm::mat4 Transform = glm::mat4(1.0f);
};

struct FLightInstance {
    uint32_t EntityId = 0;
    uint32_t LightType = 0;
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Color = glm::vec3(1.0f);
    float Intensity = 1.0f;
};

struct FActorInstance {
    uint32_t EntityId = 0;
    uint32_t ActorType = 0;
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f);
};

// ============================================================================
// Level Streaming States
// ============================================================================
enum class ELevelState : uint8_t {
    Unloaded = 0,
    Loading,
    Loaded,
    Unloading,
    Error
};

enum class ELevelPriority : uint8_t {
    Low = 0,
    Medium,
    High,
    Critical,
    AlwaysLoaded
};

// ============================================================================
// Level Chunk Information
// ============================================================================
struct FLevelChunkInfo {
    std::string ChunkId;
    std::string FilePath;
    glm::vec3 BoundsMin;
    glm::vec3 BoundsMax;
    float LoadRadius = 100.0f;
    float UnloadRadius = 150.0f;
    ELevelPriority Priority = ELevelPriority::Medium;
    std::vector<std::string> Dependencies;
    
    // Network
    uint32_t NetworkPriority = 100;
    bool bReplicateToAll = false;
    bool bCriticalForGameplay = false;
    
    // Serialization
    void Serialize(std::vector<uint8_t>& OutData) const;
    void Deserialize(const std::vector<uint8_t>& InData);
};

// ============================================================================
// Level Definition
// ============================================================================
struct FLevelDefinition {
    std::string LevelName;
    std::string Version;
    glm::vec3 WorldBoundsMin;
    glm::vec3 WorldBoundsMax;
    std::vector<FLevelChunkInfo> Chunks;
    
    // Server settings
    uint32_t MaxPlayers = 64;
    float StreamingRadius = 200.0f;
    bool bUseDistanceBasedStreaming = true;
    bool bUsePriorityBasedStreaming = true;
    uint32_t MaxConcurrentLoads = 4;
    
    // Performance
    float MemoryBudgetMB = 2048.0f;
    float TargetFrameTime = 16.6f; // 60 FPS
};

// ============================================================================
// Level Chunk Implementation
// ============================================================================
class FLevelChunk {
public:
    FLevelChunk(const FLevelChunkInfo& InInfo);
    ~FLevelChunk();
    
    // Lifecycle
    void LoadAsync(std::function<void(bool)> OnComplete = nullptr);
    void UnloadAsync(std::function<void(bool)> OnComplete = nullptr);
    void UpdateStreaming(const glm::vec3& PlayerPosition);
    void Tick(float DeltaTime);
    
    // State
    ELevelState GetState() const { return State.load(); }
    bool IsLoaded() const { return State == ELevelState::Loaded; }
    bool IsStreaming() const { return State == ELevelState::Loading || State == ELevelState::Unloading; }
    float GetLoadProgress() const { return LoadProgress.load(); }
    
    // Network
    void SerializeForNetwork(std::vector<uint8_t>& OutData) const;
    void DeserializeFromNetwork(const std::vector<uint8_t>& InData);
    uint32_t GetNetworkChecksum() const;
    
    // Getters
    const FLevelChunkInfo& GetInfo() const { return Info; }
    const std::string& GetChunkId() const { return Info.ChunkId; }
    bool IsInRange(const glm::vec3& Position) const;
    
    // Events
    using FOnLoadComplete = std::function<void(FLevelChunk*, bool)>;
    using FOnUnloadComplete = std::function<void(FLevelChunk*, bool)>;
    using FOnStateChanged = std::function<void(FLevelChunk*, ELevelState, ELevelState)>;
    
    void SetOnLoadComplete(FOnLoadComplete Callback) { OnLoadComplete = Callback; }
    void SetOnUnloadComplete(FOnUnloadComplete Callback) { OnUnloadComplete = Callback; }
    void SetOnStateChanged(FOnStateChanged Callback) { OnStateChanged = Callback; }
    
    // v7.3 Integration
    void LoadV73Chunk(const V73::Chunk& ChunkData);
    const V73::Chunk* GetV73Data() const { return V73ChunkData.get(); }
    
private:
    void PerformLoad();
    void PerformUnload();
    void UpdateDependencies();
    void ApplyLoadBudget(float DeltaTime);
    
    // Data
    FLevelChunkInfo Info;
    std::atomic<ELevelState> State{ELevelState::Unloaded};
    std::atomic<float> LoadProgress{0.0f};
    
    // Async handles
    std::future<bool> LoadFuture;
    std::future<bool> UnloadFuture;
    std::mutex StateMutex;
    
    // v7.3 Integration
    std::unique_ptr<V73::Chunk> V73ChunkData;
    
    // References to loaded data
    std::vector<FMeshInstance> LoadedMeshes;
    std::vector<FLightInstance> LoadedLights;
    std::vector<FActorInstance> LoadedActors;
    
    // Events
    FOnLoadComplete OnLoadComplete;
    FOnUnloadComplete OnUnloadComplete;
    FOnStateChanged OnStateChanged;
    
    // Performance tracking
    float LoadTimeMs = 0.0f;
    size_t MemoryUsageBytes = 0;
    uint32_t FrameLastAccessed = 0;
};

// ============================================================================
// Streaming Request
// ============================================================================
struct FStreamingRequest {
    std::string ChunkId;
    glm::vec3 PlayerPosition;
    bool bLoad;
    uint32_t Priority;
    uint64_t Timestamp;
    
    bool operator<(const FStreamingRequest& Other) const {
        return Priority < Other.Priority;
    }
};

// ============================================================================
// Streaming Role
// ============================================================================
enum class EStreamingRole : uint8_t {
    Client,
    ListenServer,
    DedicatedServer
};

// ============================================================================
// Level Streaming Subsystem
// ============================================================================
class FLevelStreamingSubsystem {
public:
    static FLevelStreamingSubsystem& Get();
    
    // Initialization
    void Initialize(const FLevelDefinition& InDefinition, EStreamingRole InRole);
    void Shutdown();
    
    // Core streaming
    void Update(float DeltaTime);
    void ProcessStreamingRequests();
    void UpdatePlayerPositions(const std::unordered_map<uint32_t, glm::vec3>& PlayerPositions);
    
    // Chunk management
    std::shared_ptr<FLevelChunk> GetChunk(const std::string& ChunkId);
    void RequestLoadChunk(const std::string& ChunkId, uint32_t Priority = 100);
    void RequestUnloadChunk(const std::string& ChunkId);
    
    // Network replication (Server)
    void ReplicateChunkStates(std::vector<uint8_t>& OutData, uint32_t ClientId);
    void ProcessReplicatedChunkStates(const std::vector<uint8_t>& InData);
    
    // Server authority
    void SetServerAuthority(bool bAuthority) { bIsServerAuthority = bAuthority; }
    bool HasServerAuthority() const { return bIsServerAuthority; }
    
    // Query
    std::vector<std::shared_ptr<FLevelChunk>> GetLoadedChunks() const;
    float GetTotalMemoryUsage() const;
    int GetStreamingQueueSize() const;
    
    // Events
    using FOnChunkLoaded = std::function<void(const std::string&)>;
    using FOnChunkUnloaded = std::function<void(const std::string&)>;
    using FOnStreamingBudgetExceeded = std::function<void(float CurrentBudget, float MaxBudget)>;
    
    void SetOnChunkLoaded(FOnChunkLoaded Callback) { OnChunkLoaded = Callback; }
    void SetOnChunkUnloaded(FOnChunkUnloaded Callback) { OnChunkUnloaded = Callback; }
    void SetOnStreamingBudgetExceeded(FOnStreamingBudgetExceeded Callback) { OnStreamingBudgetExceeded = Callback; }
    
    // v7.3 Integration
    void LoadV73Level(const V73::Level& LevelData);
    void IntegrateWithV73Manager(V73::V73LevelManager* Manager);
    
private:
    FLevelStreamingSubsystem() = default;
    ~FLevelStreamingSubsystem() = default;
    
    void UpdateStreamingBudget();
    void ProcessPriorityQueue();
    bool CanLoadChunk() const;
    bool CanUnloadChunk() const;
    void CleanupStaleRequests();
    
    // Data
    FLevelDefinition LevelDefinition;
    EStreamingRole StreamingRole = EStreamingRole::Client;
    bool bIsServerAuthority = false;
    bool bIsInitialized = false;
    
    // v7.3 Integration
    V73::V73LevelManager* V73Manager = nullptr;
    
    // Chunk storage
    std::unordered_map<std::string, std::shared_ptr<FLevelChunk>> Chunks;
    std::unordered_map<std::string, uint32_t> ChunkReferenceCounts;
    
    // Streaming management
    std::priority_queue<FStreamingRequest> LoadRequests;
    std::priority_queue<FStreamingRequest> UnloadRequests;
    std::unordered_map<std::string, FStreamingRequest> ActiveRequests;
    
    // Player tracking
    std::unordered_map<uint32_t, glm::vec3> PlayerPositions;
    uint64_t CurrentFrame = 0;
    
    // Performance tracking
    float CurrentMemoryUsageMB = 0.0f;
    float TargetMemoryBudgetMB = 2048.0f;
    int ActiveLoadCount = 0;
    int ActiveUnloadCount = 0;
    
    // Events
    FOnChunkLoaded OnChunkLoaded;
    FOnChunkUnloaded OnChunkUnloaded;
    FOnStreamingBudgetExceeded OnStreamingBudgetExceeded;
    
    // Threading
    std::mutex ChunkMutex;
    std::mutex RequestMutex;
};

// Singleton access
inline FLevelStreamingSubsystem& GLevelStreaming = FLevelStreamingSubsystem::Get();

// ============================================================================
// Player Level Manager
// ============================================================================
struct FPlayerStreamingData {
    uint32_t PlayerId;
    glm::vec3 Position;
    glm::vec3 Velocity;
    float LoadRadius = 100.0f;
    float UnloadRadius = 150.0f;
    bool bIsSpectator = false;
    bool bIsConnected = true;
};

class FPlayerLevelManager {
public:
    FPlayerLevelManager(uint32_t InPlayerId);
    ~FPlayerLevelManager();
    
    // Update
    void Tick(float DeltaTime);
    void UpdatePosition(const glm::vec3& NewPosition);
    void UpdateVelocity(const glm::vec3& NewVelocity);
    
    // Streaming
    void SetStreamingRadius(float LoadRadius, float UnloadRadius);
    float GetLoadRadius() const { return StreamingData.LoadRadius; }
    
    // Network
    void SendStreamingUpdateToServer();
    void ReceiveStreamingUpdateFromServer(const std::vector<uint8_t>& Data);
    
    // Priority
    void SetPriorityBoost(float Boost) { PriorityMultiplier = Boost; }
    float GetPriorityMultiplier() const { return PriorityMultiplier; }
    
    // Events
    using FOnChunkVisibilityChanged = std::function<void(const std::string& ChunkId, bool bVisible)>;
    void SetOnChunkVisibilityChanged(FOnChunkVisibilityChanged Callback) { OnChunkVisibilityChanged = Callback; }
    
private:
    void CalculateVisibleChunks();
    void SendVisibilityUpdate();
    
    // Data
    FPlayerStreamingData StreamingData;
    
    // Visible chunks
    std::unordered_map<std::string, bool> CurrentVisibleChunks;
    std::unordered_map<std::string, bool> PreviousVisibleChunks;
    
    // Priority
    float PriorityMultiplier = 1.0f;
    
    // Network update rate
    float NetworkUpdateTimer = 0.0f;
    static constexpr float NETWORK_UPDATE_INTERVAL = 0.1f; // 10Hz
    
    // Events
    FOnChunkVisibilityChanged OnChunkVisibilityChanged;
};

} // namespace SecretEngine::Levels::Streaming