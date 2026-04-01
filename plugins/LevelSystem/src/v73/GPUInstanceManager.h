// SecretEngine - GPU Instance Manager
// High-Performance Batched Rendering for Android
// Cache-optimized, GPU-driven culling, Memory-efficient

#pragma once
#include "V73CoreTypes.h"
#include <unordered_map>
#include <vector>
#include <span>
#include <memory>
#include <atomic>
#include <mutex>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// GPU Instance Manager - Core Performance System
// ============================================================================
class GPUInstanceManager {
public:
    // Instance group for batched rendering (same mesh + material)
    struct InstanceGroup {
        uint32_t meshId;
        uint32_t materialId;
        std::vector<TransformGPU> instances;
        std::vector<uint32_t> activeIndices;  // Indices of visible instances
        
        // GPU buffer handles (platform-specific)
        void* gpuBuffer = nullptr;
        size_t bufferSize = 0;
        bool isDirty = true;
        
        // Performance metrics
        uint32_t drawCalls = 0;
        uint32_t triangleCount = 0;
        
        void UpdateGPUData();
        size_t GetVisibleCount() const { return activeIndices.size(); }
        size_t GetMemoryUsage() const { return instances.size() * sizeof(TransformGPU); }
    };
    
    GPUInstanceManager(size_t maxInstances = V73Config::MAX_CHUNKS * V73Config::MAX_INSTANCES_PER_CHUNK);
    ~GPUInstanceManager();
    
    // Disable copy/move for thread safety
    GPUInstanceManager(const GPUInstanceManager&) = delete;
    GPUInstanceManager& operator=(const GPUInstanceManager&) = delete;
    
    // ========================================================================
    // Instance Lifecycle
    // ========================================================================
    uint32_t CreateInstance(uint32_t meshId, uint32_t materialId, 
                           const TransformCompact& transform, 
                           const glm::vec4& color = {1,1,1,1});
    
    void UpdateInstance(uint32_t instanceId, const TransformCompact& transform);
    void UpdateInstanceColor(uint32_t instanceId, const glm::vec4& color);
    void UpdateInstanceLOD(uint32_t instanceId, uint32_t lodLevel);
    void DestroyInstance(uint32_t instanceId);
    
    // ========================================================================
    // Batch Operations (High Performance)
    // ========================================================================
    void CreateInstances(std::span<const std::tuple<uint32_t, uint32_t, TransformCompact, glm::vec4>> instances);
    void UpdateInstances(std::span<const std::pair<uint32_t, TransformCompact>> updates);
    void UpdateInstanceColors(std::span<const std::pair<uint32_t, glm::vec4>> updates);
    void DestroyInstances(std::span<const uint32_t> instanceIds);
    
    // ========================================================================
    // LOD Management
    // ========================================================================
    void UpdateLODs(const glm::vec3& cameraPosition);
    void SetLODBias(float bias) { m_lodBias = bias; }
    float GetLODBias() const { return m_lodBias; }
    
    // ========================================================================
    // Culling System (GPU-Optimized)
    // ========================================================================
    void UpdateCulling(const glm::vec3& cameraPosition, 
                      const glm::vec3& cameraForward, 
                      float fov, float nearPlane = 0.1f, float farPlane = 1000.0f);
    
    void SetViewProjectionMatrix(const glm::mat4& viewProjection);
    void EnableFrustumCulling(bool enable) { m_frustumCullingEnabled = enable; }
    void EnableOcclusionCulling(bool enable) { m_occlusionCullingEnabled = enable; }
    
    // ========================================================================
    // Rendering
    // ========================================================================
    void SubmitDrawCalls();
    void FlushPendingUpdates();
    
    // ========================================================================
    // Memory Management
    // ========================================================================
    void SetMemoryBudget(size_t bytes) { m_memoryBudget = bytes; }
    void TrimToMemoryBudget();
    void DefragmentMemory();
    
    // ========================================================================
    // Statistics & Debug
    // ========================================================================
    size_t GetInstanceCount() const { return m_instanceCount.load(); }
    size_t GetVisibleCount() const { return m_visibleCount.load(); }
    size_t GetGroupCount() const { return m_groups.size(); }
    float GetGPUMemoryUsage() const { return m_gpuMemoryUsage.load(); }
    
    PerformanceMetrics GetMetrics() const;
    void ResetMetrics();
    
    // Debug functions
    void DebugDrawBounds() const;
    void SetDebugMode(bool enabled) { m_debugMode = enabled; }
    
private:
    // ========================================================================
    // Internal Data Structures
    // ========================================================================
    struct InstanceData {
        TransformCompact transform;
        uint32_t meshId;
        uint32_t materialId;
        glm::vec4 color;
        uint32_t lodLevel;
        uint32_t flags;
        bool isVisible;
        bool isDirty;
        uint32_t groupIndex;  // Index into m_groups
        uint32_t instanceIndex;  // Index within group
    };
    
    // Thread-safe containers
    std::unordered_map<uint32_t, InstanceData> m_instances;
    std::unordered_map<uint64_t, std::unique_ptr<InstanceGroup>> m_groups; // key = (meshId << 32) | materialId
    mutable std::mutex m_instanceMutex;
    mutable std::mutex m_groupMutex;
    
    // Pending operations (for batch processing)
    std::vector<uint32_t> m_pendingRemovals;
    std::vector<std::pair<uint32_t, TransformCompact>> m_pendingTransformUpdates;
    std::vector<std::pair<uint32_t, glm::vec4>> m_pendingColorUpdates;
    std::vector<std::pair<uint32_t, uint32_t>> m_pendingLODUpdates;
    std::mutex m_pendingMutex;
    
    // Instance ID management
    std::atomic<uint32_t> m_nextInstanceId{1};
    std::atomic<size_t> m_instanceCount{0};
    std::atomic<size_t> m_visibleCount{0};
    std::atomic<float> m_gpuMemoryUsage{0.0f};
    
    // Configuration
    size_t m_maxInstances;
    size_t m_memoryBudget = 256 * 1024 * 1024; // 256MB default
    float m_lodBias = 1.0f;
    
    // Culling state
    glm::mat4 m_viewProjectionMatrix{1.0f};
    glm::vec3 m_cameraPosition{0.0f};
    glm::vec3 m_cameraForward{0, 0, -1};
    float m_cameraFOV = 60.0f;
    float m_cameraNear = 0.1f;
    float m_cameraFar = 1000.0f;
    bool m_frustumCullingEnabled = true;
    bool m_occlusionCullingEnabled = false;
    
    // Performance tracking
    mutable PerformanceMetrics m_metrics;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    bool m_debugMode = false;
    
    // ========================================================================
    // Internal Helper Functions
    // ========================================================================
    uint64_t GetGroupKey(uint32_t meshId, uint32_t materialId) const {
        return (static_cast<uint64_t>(meshId) << 32) | materialId;
    }
    
    InstanceGroup* GetOrCreateGroup(uint32_t meshId, uint32_t materialId);
    void RemoveInstanceFromGroup(uint32_t instanceId, const InstanceData& data);
    
    // Batch processing
    void ProcessPendingRemovals();
    void ProcessPendingUpdates();
    void RebuildDirtyGroups();
    
    // Culling algorithms
    void PerformFrustumCulling();
    void PerformDistanceCulling(float maxDistance);
    void PerformOcclusionCulling();
    
    // Memory management
    void UpdateMemoryUsage();
    void UnloadLeastRecentlyUsedGroups();
    
    // GPU operations (platform-specific implementations)
    void CreateGPUBuffer(InstanceGroup* group);
    void UpdateGPUBuffer(InstanceGroup* group);
    void DestroyGPUBuffer(InstanceGroup* group);
    void SubmitGroupDrawCall(const InstanceGroup* group);
    
    // Sorting and optimization
    void SortGroupsByMaterial();
    void OptimizeInstanceOrder();
    
    // Debug helpers
    void ValidateInternalState() const;
    void LogPerformanceWarnings() const;
};

// ============================================================================
// Inline Implementations (Performance Critical)
// ============================================================================

inline uint32_t GPUInstanceManager::CreateInstance(uint32_t meshId, uint32_t materialId, 
                                                  const TransformCompact& transform, 
                                                  const glm::vec4& color) {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    if (m_instanceCount.load() >= m_maxInstances) {
        return 0; // Failed - at capacity
    }
    
    uint32_t instanceId = m_nextInstanceId.fetch_add(1);
    
    InstanceData data;
    data.transform = transform;
    data.meshId = meshId;
    data.materialId = materialId;
    data.color = color;
    data.lodLevel = 0;
    data.flags = TransformGPU::VISIBLE | TransformGPU::CAST_SHADOW | TransformGPU::RECEIVE_SHADOW;
    data.isVisible = true;
    data.isDirty = true;
    
    // Get or create group
    InstanceGroup* group = GetOrCreateGroup(meshId, materialId);
    if (!group) {
        return 0; // Failed to create group
    }
    
    // Add to group
    data.groupIndex = static_cast<uint32_t>(std::distance(m_groups.begin(), 
        m_groups.find(GetGroupKey(meshId, materialId))));
    data.instanceIndex = static_cast<uint32_t>(group->instances.size());
    
    TransformGPU gpuData = transform.ToGPU(meshId, materialId, 0, color, data.flags);
    gpuData.instanceId = instanceId;
    
    group->instances.push_back(gpuData);
    group->activeIndices.push_back(data.instanceIndex);
    group->isDirty = true;
    
    m_instances[instanceId] = data;
    m_instanceCount.fetch_add(1);
    m_visibleCount.fetch_add(1);
    
    return instanceId;
}

inline void GPUInstanceManager::UpdateInstance(uint32_t instanceId, const TransformCompact& transform) {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingTransformUpdates.emplace_back(instanceId, transform);
}

inline void GPUInstanceManager::UpdateInstanceColor(uint32_t instanceId, const glm::vec4& color) {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingColorUpdates.emplace_back(instanceId, color);
}

inline void GPUInstanceManager::DestroyInstance(uint32_t instanceId) {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingRemovals.push_back(instanceId);
}

} // namespace SecretEngine::Levels::V73