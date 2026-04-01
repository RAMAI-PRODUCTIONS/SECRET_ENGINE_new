// SecretEngine - Advanced Level Streaming System Implementation

#include "LevelStreamingSubsystem.h"
#include <SecretEngine/IAssetProvider.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

namespace SecretEngine::Levels::Streaming {

using json = nlohmann::json;

// ============================================================================
// FLevelChunkInfo Implementation
// ============================================================================
void FLevelChunkInfo::Serialize(std::vector<uint8_t>& OutData) const {
    // Simple binary serialization
    size_t startSize = OutData.size();
    
    // ChunkId
    uint32_t idSize = static_cast<uint32_t>(ChunkId.size());
    OutData.resize(OutData.size() + sizeof(uint32_t) + idSize);
    memcpy(OutData.data() + startSize, &idSize, sizeof(uint32_t));
    memcpy(OutData.data() + startSize + sizeof(uint32_t), ChunkId.c_str(), idSize);
    
    // Bounds
    OutData.resize(OutData.size() + sizeof(glm::vec3) * 2);
    memcpy(OutData.data() + OutData.size() - sizeof(glm::vec3) * 2, &BoundsMin, sizeof(glm::vec3));
    memcpy(OutData.data() + OutData.size() - sizeof(glm::vec3), &BoundsMax, sizeof(glm::vec3));
    
    // Other data
    OutData.resize(OutData.size() + sizeof(float) * 2 + sizeof(ELevelPriority) + sizeof(uint32_t) + sizeof(bool) * 2);
    size_t offset = OutData.size() - (sizeof(float) * 2 + sizeof(ELevelPriority) + sizeof(uint32_t) + sizeof(bool) * 2);
    memcpy(OutData.data() + offset, &LoadRadius, sizeof(float));
    offset += sizeof(float);
    memcpy(OutData.data() + offset, &UnloadRadius, sizeof(float));
    offset += sizeof(float);
    memcpy(OutData.data() + offset, &Priority, sizeof(ELevelPriority));
    offset += sizeof(ELevelPriority);
    memcpy(OutData.data() + offset, &NetworkPriority, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(OutData.data() + offset, &bReplicateToAll, sizeof(bool));
    offset += sizeof(bool);
    memcpy(OutData.data() + offset, &bCriticalForGameplay, sizeof(bool));
}

void FLevelChunkInfo::Deserialize(const std::vector<uint8_t>& InData) {
    if (InData.empty()) return;
    
    size_t offset = 0;
    
    // ChunkId
    uint32_t idSize;
    memcpy(&idSize, InData.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    ChunkId.resize(idSize);
    memcpy(ChunkId.data(), InData.data() + offset, idSize);
    offset += idSize;
    
    // Bounds
    memcpy(&BoundsMin, InData.data() + offset, sizeof(glm::vec3));
    offset += sizeof(glm::vec3);
    memcpy(&BoundsMax, InData.data() + offset, sizeof(glm::vec3));
    offset += sizeof(glm::vec3);
    
    // Other data
    memcpy(&LoadRadius, InData.data() + offset, sizeof(float));
    offset += sizeof(float);
    memcpy(&UnloadRadius, InData.data() + offset, sizeof(float));
    offset += sizeof(float);
    memcpy(&Priority, InData.data() + offset, sizeof(ELevelPriority));
    offset += sizeof(ELevelPriority);
    memcpy(&NetworkPriority, InData.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&bReplicateToAll, InData.data() + offset, sizeof(bool));
    offset += sizeof(bool);
    memcpy(&bCriticalForGameplay, InData.data() + offset, sizeof(bool));
}

// ============================================================================
// FLevelChunk Implementation
// ============================================================================
FLevelChunk::FLevelChunk(const FLevelChunkInfo& InInfo)
    : Info(InInfo)
{
}

FLevelChunk::~FLevelChunk() {
    if (State != ELevelState::Unloaded) {
        PerformUnload();
    }
}

void FLevelChunk::LoadAsync(std::function<void(bool)> OnComplete) {
    std::lock_guard<std::mutex> Lock(StateMutex);
    
    if (State != ELevelState::Unloaded) {
        if (OnComplete) OnComplete(false);
        return;
    }
    
    State = ELevelState::Loading;
    LoadProgress = 0.0f;
    
    // Start async loading
    LoadFuture = std::async(std::launch::async, [this, OnComplete]() -> bool {
        auto StartTime = std::chrono::high_resolution_clock::now();
        
        try {
            PerformLoad();
            
            auto EndTime = std::chrono::high_resolution_clock::now();
            LoadTimeMs = std::chrono::duration<float, std::milli>(EndTime - StartTime).count();
            
            State = ELevelState::Loaded;
            LoadProgress = 1.0f;
            
            if (OnComplete) OnComplete(true);
            if (OnLoadComplete) OnLoadComplete(this, true);
            
            return true;
        } catch (const std::exception& e) {
            State = ELevelState::Error;
            if (OnComplete) OnComplete(false);
            if (OnLoadComplete) OnLoadComplete(this, false);
            return false;
        }
    });
}

void FLevelChunk::PerformLoad() {
    // Read chunk file
    std::ifstream File(Info.FilePath);
    if (!File.is_open()) {
        throw std::runtime_error("Failed to open chunk file: " + Info.FilePath);
    }
    
    json ChunkData;
    File >> ChunkData;
    File.close();
    
    // Convert to v7.3 format if needed
    if (ChunkData.contains("chunk_id")) {
        V73ChunkData = std::make_unique<V73::Chunk>();
        
        // Parse v7.3 chunk data
        V73ChunkData->chunk_id = ChunkData.value("chunk_id", "unknown");
        
        // Parse bounds
        if (ChunkData.contains("bounds")) {
            const auto& bounds = ChunkData["bounds"];
            if (bounds.contains("min") && bounds["min"].is_array()) {
                auto min_bounds = bounds["min"];
                V73ChunkData->bounds_min = glm::vec3(
                    min_bounds[0].get<float>(),
                    min_bounds[1].get<float>(),
                    min_bounds[2].get<float>()
                );
            }
            if (bounds.contains("max") && bounds["max"].is_array()) {
                auto max_bounds = bounds["max"];
                V73ChunkData->bounds_max = glm::vec3(
                    max_bounds[0].get<float>(),
                    max_bounds[1].get<float>(),
                    max_bounds[2].get<float>()
                );
            }
            if (bounds.contains("center") && bounds["center"].is_array()) {
                auto center = bounds["center"];
                V73ChunkData->bounds_center = glm::vec3(
                    center[0].get<float>(),
                    center[1].get<float>(),
                    center[2].get<float>()
                );
            }
        }
        
        // Parse mesh groups
        if (ChunkData.contains("mesh_groups") && ChunkData["mesh_groups"].is_array()) {
            for (const auto& meshGroup : ChunkData["mesh_groups"]) {
                std::string mesh = meshGroup.value("mesh", "");
                std::string material = meshGroup.value("material", "");
                
                if (meshGroup.contains("instances") && meshGroup["instances"].is_array()) {
                    for (const auto& instance : meshGroup["instances"]) {
                        V73::MeshInstance meshInstance;
                        meshInstance.id = static_cast<uint32_t>(V73ChunkData->mesh_instances.size());
                        
                        // Parse transform
                        if (instance.contains("transform")) {
                            const auto& transform = instance["transform"];
                            
                            // Position
                            if (transform.contains("position") && transform["position"].is_array()) {
                                auto pos = transform["position"];
                                meshInstance.transform.position = glm::vec3(
                                    pos[0].get<float>(),
                                    pos[1].get<float>(),
                                    pos[2].get<float>()
                                );
                            }
                            
                            // Rotation (YXZ order, degrees)
                            if (transform.contains("rotation") && transform["rotation"].is_array()) {
                                auto rot = transform["rotation"];
                                glm::vec3 eulerDegrees(
                                    rot[0].get<float>(),
                                    rot[1].get<float>(),
                                    rot[2].get<float>()
                                );
                                meshInstance.transform = V73::Transform::FromEulerYXZ(
                                    meshInstance.transform.position, eulerDegrees, meshInstance.transform.scale);
                            }
                            
                            // Scale
                            if (transform.contains("scale") && transform["scale"].is_array()) {
                                auto scale = transform["scale"];
                                meshInstance.transform.scale = glm::vec3(
                                    scale[0].get<float>(),
                                    scale[1].get<float>(),
                                    scale[2].get<float>()
                                );
                            }
                        }
                        
                        // Parse material params
                        if (instance.contains("material_params")) {
                            const auto& params = instance["material_params"];
                            if (params.contains("color") && params["color"].is_array()) {
                                auto color = params["color"];
                                meshInstance.color = glm::vec4(
                                    color[0].get<float>(),
                                    color[1].get<float>(),
                                    color[2].get<float>(),
                                    1.0f
                                );
                            }
                        }
                        
                        // Parse culling
                        if (instance.contains("culling")) {
                            const auto& culling = instance["culling"];
                            meshInstance.culling.radius = culling.value("radius", 1.0f);
                        }
                        
                        V73ChunkData->mesh_instances.push_back(meshInstance);
                        
                        // Simulate loading progress
                        LoadProgress = static_cast<float>(V73ChunkData->mesh_instances.size()) / 
                                     static_cast<float>(meshGroup["instances"].size());
                        
                        // Simulate heavy load
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        
                        // Track memory
                        MemoryUsageBytes += 1024 * 1024; // Simulate 1MB per mesh
                    }
                }
            }
        }
    }
    
    V73ChunkData->is_loaded = true;
}

void FLevelChunk::UnloadAsync(std::function<void(bool)> OnComplete) {
    std::lock_guard<std::mutex> Lock(StateMutex);
    
    if (State != ELevelState::Loaded) {
        if (OnComplete) OnComplete(false);
        return;
    }
    
    State = ELevelState::Unloading;
    
    UnloadFuture = std::async(std::launch::async, [this, OnComplete]() -> bool {
        PerformUnload();
        
        State = ELevelState::Unloaded;
        MemoryUsageBytes = 0;
        
        if (OnComplete) OnComplete(true);
        if (OnUnloadComplete) OnUnloadComplete(this, true);
        
        return true;
    });
}

void FLevelChunk::PerformUnload() {
    // Clear all loaded data
    LoadedMeshes.clear();
    LoadedLights.clear();
    LoadedActors.clear();
    
    // Clear v7.3 data
    if (V73ChunkData) {
        V73ChunkData->is_loaded = false;
        V73ChunkData.reset();
    }
    
    // Force memory deallocation
    LoadedMeshes.shrink_to_fit();
    LoadedLights.shrink_to_fit();
    LoadedActors.shrink_to_fit();
    
    // Simulate unload time
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void FLevelChunk::UpdateStreaming(const glm::vec3& PlayerPosition) {
    if (!IsLoaded()) return;
    
    // Check if player is within range
    bool bInRange = IsInRange(PlayerPosition);
    if (!bInRange) {
        // Auto-unload if out of range and not critical
        if (Info.Priority != ELevelPriority::AlwaysLoaded && State == ELevelState::Loaded) {
            UnloadAsync();
        }
    }
}

bool FLevelChunk::IsInRange(const glm::vec3& Position) const {
    // Check if position is within chunk bounds
    if (Position.x < Info.BoundsMin.x || Position.x > Info.BoundsMax.x) return false;
    if (Position.z < Info.BoundsMin.z || Position.z > Info.BoundsMax.z) return false;
    return true;
}

void FLevelChunk::Tick(float DeltaTime) {
    // Update streaming progress
    if (State == ELevelState::Loading && LoadProgress < 1.0f) {
        // Simulate progressive loading
        LoadProgress += DeltaTime * 0.5f;
        if (LoadProgress >= 1.0f) LoadProgress = 1.0f;
    }
}

void FLevelChunk::SerializeForNetwork(std::vector<uint8_t>& OutData) const {
    // Simple serialization for network replication
    size_t CurrentSize = OutData.size();
    OutData.resize(CurrentSize + sizeof(ELevelState) + sizeof(float) + sizeof(uint32_t));
    
    uint8_t* Data = OutData.data() + CurrentSize;
    memcpy(Data, &State, sizeof(ELevelState));
    Data += sizeof(ELevelState);
    
    float progress = LoadProgress.load();
    memcpy(Data, &progress, sizeof(float));
    Data += sizeof(float);
    
    uint32_t Checksum = GetNetworkChecksum();
    memcpy(Data, &Checksum, sizeof(uint32_t));
}

void FLevelChunk::DeserializeFromNetwork(const std::vector<uint8_t>& InData) {
    if (InData.size() < sizeof(ELevelState) + sizeof(float) + sizeof(uint32_t)) return;
    
    const uint8_t* Data = InData.data();
    
    ELevelState NewState;
    float NewProgress;
    uint32_t ReceivedChecksum;
    
    memcpy(&NewState, Data, sizeof(ELevelState));
    Data += sizeof(ELevelState);
    
    memcpy(&NewProgress, Data, sizeof(float));
    Data += sizeof(float);
    
    memcpy(&ReceivedChecksum, Data, sizeof(uint32_t));
    
    if (ReceivedChecksum == GetNetworkChecksum()) {
        State = NewState;
        LoadProgress = NewProgress;
    }
}

uint32_t FLevelChunk::GetNetworkChecksum() const {
    // Simple checksum for verification
    uint32_t Checksum = 0;
    for (char c : Info.ChunkId) {
        Checksum = (Checksum << 5) - Checksum + c;
    }
    return Checksum;
}

void FLevelChunk::LoadV73Chunk(const V73::Chunk& ChunkData) {
    V73ChunkData = std::make_unique<V73::Chunk>(ChunkData);
    V73ChunkData->is_loaded = true;
    State = ELevelState::Loaded;
    LoadProgress = 1.0f;
}

// ============================================================================
// FLevelStreamingSubsystem Implementation
// ============================================================================
FLevelStreamingSubsystem& FLevelStreamingSubsystem::Get() {
    static FLevelStreamingSubsystem Instance;
    return Instance;
}

void FLevelStreamingSubsystem::Initialize(const FLevelDefinition& InDefinition, EStreamingRole InRole) {
    if (bIsInitialized) Shutdown();
    
    LevelDefinition = InDefinition;
    StreamingRole = InRole;
    bIsServerAuthority = (InRole == EStreamingRole::DedicatedServer || InRole == EStreamingRole::ListenServer);
    bIsInitialized = true;
    
    // Create chunks
    for (const auto& ChunkInfo : LevelDefinition.Chunks) {
        auto Chunk = std::make_shared<FLevelChunk>(ChunkInfo);
        Chunks[ChunkInfo.ChunkId] = Chunk;
        
        // Always load critical chunks on server
        if (bIsServerAuthority && ChunkInfo.Priority == ELevelPriority::AlwaysLoaded) {
            RequestLoadChunk(ChunkInfo.ChunkId, 1000);
        }
    }
    
    // Set memory budget
    TargetMemoryBudgetMB = LevelDefinition.MemoryBudgetMB;
}

void FLevelStreamingSubsystem::Shutdown() {
    std::lock_guard<std::mutex> Lock(ChunkMutex);
    
    // Unload all chunks
    for (auto& [Id, Chunk] : Chunks) {
        if (Chunk->IsLoaded()) {
            Chunk->UnloadAsync();
        }
    }
    
    Chunks.clear();
    LoadRequests = {};
    UnloadRequests = {};
    ActiveRequests.clear();
    bIsInitialized = false;
}

void FLevelStreamingSubsystem::Update(float DeltaTime) {
    if (!bIsInitialized) return;
    
    CurrentFrame++;
    
    // Update all chunks
    {
        std::lock_guard<std::mutex> Lock(ChunkMutex);
        for (auto& [Id, Chunk] : Chunks) {
            Chunk->Tick(DeltaTime);
            
            // Update streaming based on player positions
            if (StreamingRole != EStreamingRole::DedicatedServer) {
                for (const auto& [PlayerId, Position] : PlayerPositions) {
                    Chunk->UpdateStreaming(Position);
                }
            }
        }
    }
    
    // Process streaming requests
    ProcessStreamingRequests();
    
    // Update streaming budget
    UpdateStreamingBudget();
}

void FLevelStreamingSubsystem::UpdatePlayerPositions(const std::unordered_map<uint32_t, glm::vec3>& Positions) {
    PlayerPositions = Positions;
    
    if (!bIsServerAuthority) return;
    
    // Server: evaluate which chunks should be loaded based on player positions
    std::unordered_map<std::string, int> ChunkPriority;
    
    for (const auto& [PlayerId, Position] : PlayerPositions) {
        for (const auto& [ChunkId, Chunk] : Chunks) {
            if (Chunk->IsInRange(Position)) {
                float Distance = glm::distance(Position, Chunk->GetInfo().BoundsMin);
                int Priority = static_cast<int>(1000.0f / (Distance + 1.0f));
                ChunkPriority[ChunkId] += Priority;
            }
        }
    }
    
    // Request chunks based on priority
    for (const auto& [ChunkId, Priority] : ChunkPriority) {
        auto It = Chunks.find(ChunkId);
        if (It != Chunks.end() && !It->second->IsLoaded()) {
            RequestLoadChunk(ChunkId, Priority);
        }
    }
}

void FLevelStreamingSubsystem::RequestLoadChunk(const std::string& ChunkId, uint32_t Priority) {
    auto It = Chunks.find(ChunkId);
    if (It == Chunks.end()) return;
    
    auto& Chunk = It->second;
    if (Chunk->IsLoaded() || Chunk->IsStreaming()) return;
    
    std::lock_guard<std::mutex> Lock(RequestMutex);
    
    FStreamingRequest Request;
    Request.ChunkId = ChunkId;
    Request.bLoad = true;
    Request.Priority = Priority;
    Request.Timestamp = CurrentFrame;
    
    LoadRequests.push(Request);
    ActiveRequests[ChunkId] = Request;
}

void FLevelStreamingSubsystem::RequestUnloadChunk(const std::string& ChunkId) {
    auto It = Chunks.find(ChunkId);
    if (It == Chunks.end()) return;
    
    auto& Chunk = It->second;
    if (!Chunk->IsLoaded() || Chunk->IsStreaming()) return;
    
    std::lock_guard<std::mutex> Lock(RequestMutex);
    
    FStreamingRequest Request;
    Request.ChunkId = ChunkId;
    Request.bLoad = false;
    Request.Priority = 0;
    Request.Timestamp = CurrentFrame;
    
    UnloadRequests.push(Request);
    ActiveRequests.erase(ChunkId);
}

void FLevelStreamingSubsystem::ProcessStreamingRequests() {
    std::lock_guard<std::mutex> Lock(RequestMutex);
    
    // Process loads
    while (!LoadRequests.empty() && CanLoadChunk()) {
        auto Request = LoadRequests.top();
        LoadRequests.pop();
        
        auto It = Chunks.find(Request.ChunkId);
        if (It != Chunks.end()) {
            auto& Chunk = It->second;
            if (!Chunk->IsLoaded() && !Chunk->IsStreaming()) {
                ActiveLoadCount++;
                Chunk->LoadAsync([this, ChunkId = Request.ChunkId](bool bSuccess) {
                    ActiveLoadCount--;
                    if (bSuccess && OnChunkLoaded) {
                        OnChunkLoaded(ChunkId);
                    }
                });
            }
        }
    }
    
    // Process unloads
    while (!UnloadRequests.empty() && CanUnloadChunk()) {
        auto Request = UnloadRequests.top();
        UnloadRequests.pop();
        
        auto It = Chunks.find(Request.ChunkId);
        if (It != Chunks.end()) {
            auto& Chunk = It->second;
            if (Chunk->IsLoaded() && !Chunk->IsStreaming()) {
                ActiveUnloadCount++;
                Chunk->UnloadAsync([this, ChunkId = Request.ChunkId](bool bSuccess) {
                    ActiveUnloadCount--;
                    if (bSuccess && OnChunkUnloaded) {
                        OnChunkUnloaded(ChunkId);
                    }
                });
            }
        }
    }
}

bool FLevelStreamingSubsystem::CanLoadChunk() const {
    // Check concurrent load limit
    if (ActiveLoadCount >= static_cast<int>(LevelDefinition.MaxConcurrentLoads))
        return false;
    
    // Check memory budget
    if (CurrentMemoryUsageMB + 100.0f > TargetMemoryBudgetMB)
        return false;
    
    return true;
}

bool FLevelStreamingSubsystem::CanUnloadChunk() const {
    // Limit concurrent unloads to avoid stuttering
    return ActiveUnloadCount < 2;
}

void FLevelStreamingSubsystem::UpdateStreamingBudget() {
    // Calculate current memory usage
    float NewMemoryUsage = 0.0f;
    for (const auto& [Id, Chunk] : Chunks) {
        if (Chunk->IsLoaded()) {
            NewMemoryUsage += Chunk->GetInfo().Priority == ELevelPriority::AlwaysLoaded ? 500.0f : 100.0f;
        }
    }
    
    CurrentMemoryUsageMB = NewMemoryUsage;
    
    // Trigger event if budget exceeded
    if (CurrentMemoryUsageMB > TargetMemoryBudgetMB && OnStreamingBudgetExceeded) {
        OnStreamingBudgetExceeded(CurrentMemoryUsageMB, TargetMemoryBudgetMB);
    }
}

std::vector<std::shared_ptr<FLevelChunk>> FLevelStreamingSubsystem::GetLoadedChunks() const {
    std::vector<std::shared_ptr<FLevelChunk>> Result;
    for (const auto& [Id, Chunk] : Chunks) {
        if (Chunk->IsLoaded()) {
            Result.push_back(Chunk);
        }
    }
    return Result;
}

void FLevelStreamingSubsystem::LoadV73Level(const V73::Level& LevelData) {
    // Convert v7.3 level to streaming format
    FLevelDefinition StreamingDef;
    StreamingDef.LevelName = LevelData.name;
    StreamingDef.Version = LevelData.version;
    StreamingDef.WorldBoundsMin = LevelData.boundaries.playable_min;
    StreamingDef.WorldBoundsMax = LevelData.boundaries.playable_max;
    StreamingDef.MaxPlayers = LevelData.server_config.max_players;
    StreamingDef.MemoryBudgetMB = LevelData.performance_budgets.max_visible_instances * 0.1f; // Estimate
    
    // Convert chunks
    for (const auto& v73Chunk : LevelData.chunks) {
        FLevelChunkInfo ChunkInfo;
        ChunkInfo.ChunkId = v73Chunk.chunk_id;
        ChunkInfo.BoundsMin = v73Chunk.bounds_min;
        ChunkInfo.BoundsMax = v73Chunk.bounds_max;
        ChunkInfo.LoadRadius = v73Chunk.streaming.preload_radius;
        ChunkInfo.UnloadRadius = ChunkInfo.LoadRadius * 1.5f;
        ChunkInfo.Priority = static_cast<ELevelPriority>(v73Chunk.streaming.priority);
        
        StreamingDef.Chunks.push_back(ChunkInfo);
    }
    
    Initialize(StreamingDef, EStreamingRole::Client);
    
    // Load v7.3 chunks directly
    for (const auto& v73Chunk : LevelData.chunks) {
        auto It = Chunks.find(v73Chunk.chunk_id);
        if (It != Chunks.end()) {
            It->second->LoadV73Chunk(v73Chunk);
        }
    }
}

void FLevelStreamingSubsystem::IntegrateWithV73Manager(V73::V73LevelManager* Manager) {
    V73Manager = Manager;
}

// ============================================================================
// FPlayerLevelManager Implementation
// ============================================================================
FPlayerLevelManager::FPlayerLevelManager(uint32_t InPlayerId) {
    StreamingData.PlayerId = InPlayerId;
}

FPlayerLevelManager::~FPlayerLevelManager() = default;

void FPlayerLevelManager::Tick(float DeltaTime) {
    NetworkUpdateTimer += DeltaTime;
    
    if (NetworkUpdateTimer >= NETWORK_UPDATE_INTERVAL) {
        CalculateVisibleChunks();
        SendVisibilityUpdate();
        NetworkUpdateTimer = 0.0f;
    }
}

void FPlayerLevelManager::UpdatePosition(const glm::vec3& NewPosition) {
    StreamingData.Position = NewPosition;
}

void FPlayerLevelManager::UpdateVelocity(const glm::vec3& NewVelocity) {
    StreamingData.Velocity = NewVelocity;
}

void FPlayerLevelManager::SetStreamingRadius(float LoadRadius, float UnloadRadius) {
    StreamingData.LoadRadius = LoadRadius;
    StreamingData.UnloadRadius = UnloadRadius;
}

void FPlayerLevelManager::CalculateVisibleChunks() {
    PreviousVisibleChunks = CurrentVisibleChunks;
    CurrentVisibleChunks.clear();
    
    // Get all chunks from streaming system
    auto LoadedChunks = GLevelStreaming.GetLoadedChunks();
    
    for (const auto& Chunk : LoadedChunks) {
        bool bVisible = Chunk->IsInRange(StreamingData.Position);
        CurrentVisibleChunks[Chunk->GetChunkId()] = bVisible;
        
        // Check if visibility changed
        auto PrevIt = PreviousVisibleChunks.find(Chunk->GetChunkId());
        bool bPreviouslyVisible = (PrevIt != PreviousVisibleChunks.end()) ? PrevIt->second : false;
        
        if (bVisible != bPreviouslyVisible && OnChunkVisibilityChanged) {
            OnChunkVisibilityChanged(Chunk->GetChunkId(), bVisible);
        }
    }
}

void FPlayerLevelManager::SendVisibilityUpdate() {
    // TODO: Send visibility update to server
}

void FPlayerLevelManager::SendStreamingUpdateToServer() {
    // TODO: Implement network communication
}

void FPlayerLevelManager::ReceiveStreamingUpdateFromServer(const std::vector<uint8_t>& Data) {
    // TODO: Process server streaming update
}

} // namespace SecretEngine::Levels::Streaming