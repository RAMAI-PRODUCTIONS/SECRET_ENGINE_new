// SecretEngine - GPU Instance Manager Implementation
// High-Performance Batched Rendering for Android

#include "GPUInstanceManager.h"
#include <algorithm>
#include <execution>
#include <chrono>

namespace SecretEngine::Levels::V73 {

// ============================================================================
// Constructor / Destructor
// ============================================================================
GPUInstanceManager::GPUInstanceManager(size_t maxInstances) 
    : m_maxInstances(maxInstances)
    , m_lastUpdateTime(std::chrono::steady_clock::now()) {
    
    // Reserve memory for performance
    m_instances.reserve(maxInstances);
    m_pendingRemovals.reserve(1000);
    m_pendingTransformUpdates.reserve(1000);
    m_pendingColorUpdates.reserve(1000);
    m_pendingLODUpdates.reserve(1000);
}

GPUInstanceManager::~GPUInstanceManager() {
    // Clean up GPU resources
    for (auto& [key, group] : m_groups) {
        DestroyGPUBuffer(group.get());
    }
}

// ============================================================================
// Batch Operations (Critical for Android Performance)
// ============================================================================
void GPUInstanceManager::CreateInstances(
    std::span<const std::tuple<uint32_t, uint32_t, TransformCompact, glm::vec4>> instances) {
    
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    // Group instances by mesh/material for optimal batching
    std::unordered_map<uint64_t, std::vector<size_t>> groupedIndices;
    
    for (size_t i = 0; i < instances.size(); ++i) {
        auto [meshId, materialId, transform, color] = instances[i];
        uint64_t key = GetGroupKey(meshId, materialId);
        groupedIndices[key].push_back(i);
    }
    
    // Process each group
    for (auto& [groupKey, indices] : groupedIndices) {
        uint32_t meshId = static_cast<uint32_t>(groupKey >> 32);
        uint32_t materialId = static_cast<uint32_t>(groupKey & 0xFFFFFFFF);
        
        InstanceGroup* group = GetOrCreateGroup(meshId, materialId);
        if (!group) continue;
        
        // Reserve space for better performance
        size_t oldSize = group->instances.size();
        group->instances.reserve(oldSize + indices.size());
        group->activeIndices.reserve(oldSize + indices.size());
        
        // Add all instances to this group
        for (size_t idx : indices) {
            if (m_instanceCount.load() >= m_maxInstances) break;
            
            auto [meshId, materialId, transform, color] = instances[idx];
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
            data.instanceIndex = static_cast<uint32_t>(group->instances.size());
            
            TransformGPU gpuData = transform.ToGPU(meshId, materialId, 0, color, data.flags);
            gpuData.instanceId = instanceId;
            
            group->instances.push_back(gpuData);
            group->activeIndices.push_back(data.instanceIndex);
            
            m_instances[instanceId] = data;
            m_instanceCount.fetch_add(1);
            m_visibleCount.fetch_add(1);
        }
        
        group->isDirty = true;
    }
}

void GPUInstanceManager::UpdateInstances(std::span<const std::pair<uint32_t, TransformCompact>> updates) {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    // Process updates sequentially (parallel execution not available on Android C++20)
    for (const auto& update : updates) {
        auto [instanceId, transform] = update;
        
        auto it = m_instances.find(instanceId);
        if (it != m_instances.end()) {
            it->second.transform = transform;
            it->second.isDirty = true;
            
            // Update GPU data
            uint64_t groupKey = GetGroupKey(it->second.meshId, it->second.materialId);
            auto groupIt = m_groups.find(groupKey);
            if (groupIt != m_groups.end()) {
                InstanceGroup* group = groupIt->second.get();
                if (it->second.instanceIndex < group->instances.size()) {
                    TransformGPU& gpuData = group->instances[it->second.instanceIndex];
                    gpuData.modelMatrix = transform.ToMatrix();
                    group->isDirty = true;
                }
            }
        }
    }
}

void GPUInstanceManager::DestroyInstances(std::span<const uint32_t> instanceIds) {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    for (uint32_t instanceId : instanceIds) {
        auto it = m_instances.find(instanceId);
        if (it != m_instances.end()) {
            RemoveInstanceFromGroup(instanceId, it->second);
            m_instances.erase(it);
            m_instanceCount.fetch_sub(1);
        }
    }
}

// ============================================================================
// LOD Management (Critical for Performance)
// ============================================================================
void GPUInstanceManager::UpdateLODs(const glm::vec3& cameraPosition) {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    // Update LODs sequentially (parallel execution not available on Android C++20)
    for (auto& pair : m_instances) {
        auto& [instanceId, data] = pair;
        
        float distance = glm::distance(data.transform.position, cameraPosition);
        distance /= m_lodBias;  // Apply LOD bias
        
        uint32_t newLOD = 0;
        for (const float lodDistance : V73Config::DEFAULT_LOD_DISTANCES) {
            if (distance > lodDistance) {
                newLOD++;
            } else {
                break;
            }
        }
        
        if (newLOD != data.lodLevel) {
            data.lodLevel = newLOD;
            data.isDirty = true;
            
            // Update GPU data
            uint64_t groupKey = GetGroupKey(data.meshId, data.materialId);
            auto groupIt = m_groups.find(groupKey);
            if (groupIt != m_groups.end()) {
                InstanceGroup* group = groupIt->second.get();
                if (data.instanceIndex < group->instances.size()) {
                    group->instances[data.instanceIndex].lodLevel = newLOD;
                    group->isDirty = true;
                }
            }
        }
    }
}

// ============================================================================
// Culling System (GPU-Optimized for Android)
// ============================================================================
void GPUInstanceManager::UpdateCulling(const glm::vec3& cameraPosition, 
                                      const glm::vec3& cameraForward, 
                                      float fov, float nearPlane, float farPlane) {
    m_cameraPosition = cameraPosition;
    m_cameraForward = cameraForward;
    m_cameraFOV = fov;
    m_cameraNear = nearPlane;
    m_cameraFar = farPlane;
    
    if (m_frustumCullingEnabled) {
        PerformFrustumCulling();
    }
    
    // Distance culling (always enabled for performance)
    PerformDistanceCulling(m_cameraFar);
    
    if (m_occlusionCullingEnabled) {
        PerformOcclusionCulling();
    }
}

void GPUInstanceManager::PerformFrustumCulling() {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    // Extract frustum planes from view-projection matrix
    glm::vec4 frustumPlanes[6];
    
    // Left plane
    frustumPlanes[0] = glm::vec4(
        m_viewProjectionMatrix[0][3] + m_viewProjectionMatrix[0][0],
        m_viewProjectionMatrix[1][3] + m_viewProjectionMatrix[1][0],
        m_viewProjectionMatrix[2][3] + m_viewProjectionMatrix[2][0],
        m_viewProjectionMatrix[3][3] + m_viewProjectionMatrix[3][0]
    );
    
    // Right plane
    frustumPlanes[1] = glm::vec4(
        m_viewProjectionMatrix[0][3] - m_viewProjectionMatrix[0][0],
        m_viewProjectionMatrix[1][3] - m_viewProjectionMatrix[1][0],
        m_viewProjectionMatrix[2][3] - m_viewProjectionMatrix[2][0],
        m_viewProjectionMatrix[3][3] - m_viewProjectionMatrix[3][0]
    );
    
    // Bottom plane
    frustumPlanes[2] = glm::vec4(
        m_viewProjectionMatrix[0][3] + m_viewProjectionMatrix[0][1],
        m_viewProjectionMatrix[1][3] + m_viewProjectionMatrix[1][1],
        m_viewProjectionMatrix[2][3] + m_viewProjectionMatrix[2][1],
        m_viewProjectionMatrix[3][3] + m_viewProjectionMatrix[3][1]
    );
    
    // Top plane
    frustumPlanes[3] = glm::vec4(
        m_viewProjectionMatrix[0][3] - m_viewProjectionMatrix[0][1],
        m_viewProjectionMatrix[1][3] - m_viewProjectionMatrix[1][1],
        m_viewProjectionMatrix[2][3] - m_viewProjectionMatrix[2][1],
        m_viewProjectionMatrix[3][3] - m_viewProjectionMatrix[3][1]
    );
    
    // Near plane
    frustumPlanes[4] = glm::vec4(
        m_viewProjectionMatrix[0][3] + m_viewProjectionMatrix[0][2],
        m_viewProjectionMatrix[1][3] + m_viewProjectionMatrix[1][2],
        m_viewProjectionMatrix[2][3] + m_viewProjectionMatrix[2][2],
        m_viewProjectionMatrix[3][3] + m_viewProjectionMatrix[3][2]
    );
    
    // Far plane
    frustumPlanes[5] = glm::vec4(
        m_viewProjectionMatrix[0][3] - m_viewProjectionMatrix[0][2],
        m_viewProjectionMatrix[1][3] - m_viewProjectionMatrix[1][2],
        m_viewProjectionMatrix[2][3] - m_viewProjectionMatrix[2][2],
        m_viewProjectionMatrix[3][3] - m_viewProjectionMatrix[3][2]
    );
    
    // Normalize planes
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(frustumPlanes[i]));
        frustumPlanes[i] /= length;
    }
    
    size_t visibleCount = 0;
    
    // Test each instance against frustum
    for (auto& [instanceId, data] : m_instances) {
        bool wasVisible = data.isVisible;
        data.isVisible = true;
        
        // Simple sphere test (could be optimized with AABB)
        float radius = 1.0f; // Could be per-instance
        
        for (int i = 0; i < 6; ++i) {
            float distance = glm::dot(glm::vec3(frustumPlanes[i]), data.transform.position) + frustumPlanes[i].w;
            if (distance < -radius) {
                data.isVisible = false;
                break;
            }
        }
        
        if (data.isVisible) {
            visibleCount++;
        }
        
        // Update group if visibility changed
        if (wasVisible != data.isVisible) {
            uint64_t groupKey = GetGroupKey(data.meshId, data.materialId);
            auto groupIt = m_groups.find(groupKey);
            if (groupIt != m_groups.end()) {
                groupIt->second->isDirty = true;
            }
        }
    }
    
    m_visibleCount.store(visibleCount);
}

void GPUInstanceManager::PerformDistanceCulling(float maxDistance) {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    size_t visibleCount = 0;
    float maxDistanceSq = maxDistance * maxDistance;
    
    for (auto& [instanceId, data] : m_instances) {
        float dist = glm::distance(data.transform.position, m_cameraPosition);
        float distanceSq = dist * dist;
        bool wasVisible = data.isVisible;
        
        data.isVisible = data.isVisible && (distanceSq <= maxDistanceSq);
        
        if (data.isVisible) {
            visibleCount++;
        }
        
        // Update group if visibility changed
        if (wasVisible != data.isVisible) {
            uint64_t groupKey = GetGroupKey(data.meshId, data.materialId);
            auto groupIt = m_groups.find(groupKey);
            if (groupIt != m_groups.end()) {
                groupIt->second->isDirty = true;
            }
        }
    }
    
    m_visibleCount.store(visibleCount);
}

// ============================================================================
// Rendering
// ============================================================================
void GPUInstanceManager::SubmitDrawCalls() {
    FlushPendingUpdates();
    RebuildDirtyGroups();
    
    std::lock_guard<std::mutex> lock(m_groupMutex);
    
    m_metrics.drawCalls = 0;
    m_metrics.triangles = 0;
    
    // Sort groups by material for optimal rendering
    std::vector<InstanceGroup*> sortedGroups;
    sortedGroups.reserve(m_groups.size());
    
    for (auto& [key, group] : m_groups) {
        if (group->GetVisibleCount() > 0) {
            sortedGroups.push_back(group.get());
        }
    }
    
    std::sort(sortedGroups.begin(), sortedGroups.end(),
        [](const InstanceGroup* a, const InstanceGroup* b) {
            if (a->materialId != b->materialId) {
                return a->materialId < b->materialId;
            }
            return a->meshId < b->meshId;
        });
    
    // Submit draw calls
    for (InstanceGroup* group : sortedGroups) {
        SubmitGroupDrawCall(group);
        m_metrics.drawCalls++;
        // Triangle count would be calculated based on mesh data
    }
}

void GPUInstanceManager::FlushPendingUpdates() {
    ProcessPendingRemovals();
    ProcessPendingUpdates();
}

// ============================================================================
// Memory Management
// ============================================================================
void GPUInstanceManager::TrimToMemoryBudget() {
    UpdateMemoryUsage();
    
    if (m_gpuMemoryUsage.load() <= m_memoryBudget) {
        return; // Within budget
    }
    
    // Unload least recently used groups
    UnloadLeastRecentlyUsedGroups();
}

void GPUInstanceManager::UpdateMemoryUsage() {
    float totalMemory = 0.0f;
    
    std::lock_guard<std::mutex> lock(m_groupMutex);
    for (const auto& [key, group] : m_groups) {
        totalMemory += static_cast<float>(group->GetMemoryUsage());
    }
    
    m_gpuMemoryUsage.store(totalMemory);
}

// ============================================================================
// Performance Metrics
// ============================================================================
PerformanceMetrics GPUInstanceManager::GetMetrics() const {
    std::lock_guard<std::mutex> lock(m_instanceMutex);
    
    PerformanceMetrics metrics = m_metrics;
    metrics.visibleInstances = static_cast<uint32_t>(m_visibleCount.load());
    metrics.gpuMemoryMB = m_gpuMemoryUsage.load() / (1024.0f * 1024.0f);
    metrics.instanceMemoryBytes = m_instanceCount.load() * sizeof(InstanceData);
    
    // Calculate FPS
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdateTime);
    if (duration.count() > 0) {
        metrics.fps = 1000.0f / static_cast<float>(duration.count());
    }
    
    return metrics;
}

// ============================================================================
// Internal Helper Functions
// ============================================================================
GPUInstanceManager::InstanceGroup* GPUInstanceManager::GetOrCreateGroup(uint32_t meshId, uint32_t materialId) {
    uint64_t key = GetGroupKey(meshId, materialId);
    
    std::lock_guard<std::mutex> lock(m_groupMutex);
    
    auto it = m_groups.find(key);
    if (it != m_groups.end()) {
        return it->second.get();
    }
    
    // Create new group
    auto group = std::make_unique<InstanceGroup>();
    group->meshId = meshId;
    group->materialId = materialId;
    group->instances.reserve(1000); // Reserve for performance
    group->activeIndices.reserve(1000);
    
    InstanceGroup* groupPtr = group.get();
    m_groups[key] = std::move(group);
    
    CreateGPUBuffer(groupPtr);
    
    return groupPtr;
}

void GPUInstanceManager::ProcessPendingRemovals() {
    std::lock_guard<std::mutex> pendingLock(m_pendingMutex);
    std::lock_guard<std::mutex> instanceLock(m_instanceMutex);
    
    for (uint32_t instanceId : m_pendingRemovals) {
        auto it = m_instances.find(instanceId);
        if (it != m_instances.end()) {
            RemoveInstanceFromGroup(instanceId, it->second);
            m_instances.erase(it);
            m_instanceCount.fetch_sub(1);
        }
    }
    
    m_pendingRemovals.clear();
}

void GPUInstanceManager::ProcessPendingUpdates() {
    std::lock_guard<std::mutex> pendingLock(m_pendingMutex);
    
    // Process transform updates
    if (!m_pendingTransformUpdates.empty()) {
        UpdateInstances(m_pendingTransformUpdates);
        m_pendingTransformUpdates.clear();
    }
    
    // Process color updates
    for (const auto& [instanceId, color] : m_pendingColorUpdates) {
        auto it = m_instances.find(instanceId);
        if (it != m_instances.end()) {
            it->second.color = color;
            it->second.isDirty = true;
            
            // Update GPU data
            uint64_t groupKey = GetGroupKey(it->second.meshId, it->second.materialId);
            auto groupIt = m_groups.find(groupKey);
            if (groupIt != m_groups.end()) {
                InstanceGroup* group = groupIt->second.get();
                if (it->second.instanceIndex < group->instances.size()) {
                    group->instances[it->second.instanceIndex].color = color;
                    group->isDirty = true;
                }
            }
        }
    }
    m_pendingColorUpdates.clear();
    
    // Process LOD updates
    for (const auto& [instanceId, lodLevel] : m_pendingLODUpdates) {
        auto it = m_instances.find(instanceId);
        if (it != m_instances.end()) {
            it->second.lodLevel = lodLevel;
            it->second.isDirty = true;
            
            // Update GPU data
            uint64_t groupKey = GetGroupKey(it->second.meshId, it->second.materialId);
            auto groupIt = m_groups.find(groupKey);
            if (groupIt != m_groups.end()) {
                InstanceGroup* group = groupIt->second.get();
                if (it->second.instanceIndex < group->instances.size()) {
                    group->instances[it->second.instanceIndex].lodLevel = lodLevel;
                    group->isDirty = true;
                }
            }
        }
    }
    m_pendingLODUpdates.clear();
}

void GPUInstanceManager::RebuildDirtyGroups() {
    std::lock_guard<std::mutex> lock(m_groupMutex);
    
    for (auto& [key, group] : m_groups) {
        if (group->isDirty) {
            // Rebuild active indices based on visibility
            group->activeIndices.clear();
            
            for (size_t i = 0; i < group->instances.size(); ++i) {
                uint32_t instanceId = group->instances[i].instanceId;
                auto it = m_instances.find(instanceId);
                if (it != m_instances.end() && it->second.isVisible) {
                    group->activeIndices.push_back(static_cast<uint32_t>(i));
                }
            }
            
            UpdateGPUBuffer(group.get());
            group->isDirty = false;
        }
    }
}

// ============================================================================
// Platform-Specific GPU Operations (Stubs - implement per platform)
// ============================================================================
void GPUInstanceManager::CreateGPUBuffer(InstanceGroup* group) {
    // Platform-specific implementation
    // For OpenGL: glGenBuffers, glBindBuffer, glBufferData
    // For Vulkan: vkCreateBuffer, vkAllocateMemory, vkBindBufferMemory
    // For D3D11: CreateBuffer
}

void GPUInstanceManager::UpdateGPUBuffer(InstanceGroup* group) {
    // Platform-specific implementation
    // Update GPU buffer with current instance data
}

void GPUInstanceManager::DestroyGPUBuffer(InstanceGroup* group) {
    // Platform-specific implementation
    // Clean up GPU resources
}

void GPUInstanceManager::SubmitGroupDrawCall(const InstanceGroup* group) {
    // Platform-specific implementation
    // Submit instanced draw call to GPU
}

} // namespace SecretEngine::Levels::V73