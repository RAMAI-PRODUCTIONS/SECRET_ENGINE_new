#pragma once

#include "LightVertexCompact.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

/**
 * VertexGIManager - Manages the Vertex GI v4.0 system
 * 
 * Responsibilities:
 * - Light vertex cache management
 * - Spatial hash grid building
 * - TAGI (Temporal Accumulation GI) scheduling
 * - Compute shader dispatch
 * - ADPF-aware thermal throttling
 */
class VertexGIManager {
public:
    struct Config {
        uint32_t maxLightVertices = 65536;
        uint32_t maxMeshVertices = 524288;  // 512K
        uint32_t hashGridSize = 32;         // 32x32x32 cells
        float kernelRadius = 2.0f;
        float temporalBlend = 0.9f;         // 90% old, 10% new
        uint8_t tagiTileCount = 8;          // Update 1/8 per frame
    };
    
    VertexGIManager(VkDevice device, VkPhysicalDevice physicalDevice);
    ~VertexGIManager();
    
    // Initialization
    bool initialize(const Config& config);
    void cleanup();
    
    // Light vertex management
    void updateLightVertices(const std::vector<LightVertexCompact>& lightVertices);
    void buildSpatialHash();
    
    // Per-frame update
    void update(VkCommandBuffer cmd, uint32_t frameIndex, float thermalHeadroom);
    
    // Getters
    VkBuffer getVertexColorBuffer() const { return m_vertexColorBuffer; }
    uint32_t getVertexColorOffset() const { return 0; }
    
private:
    // Vulkan objects
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    
    // Buffers
    VkBuffer m_lightVertexBuffer;
    VkDeviceMemory m_lightVertexMemory;
    
    VkBuffer m_vertexColorBuffer;
    VkDeviceMemory m_vertexColorMemory;
    
    VkBuffer m_spatialHashBuffer;
    VkDeviceMemory m_spatialHashMemory;
    
    VkBuffer m_meshVertexBuffer;
    VkDeviceMemory m_meshVertexMemory;
    
    // Compute pipeline
    VkPipeline m_computePipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    
    // Configuration
    Config m_config;
    
    // State
    uint32_t m_lightVertexCount;
    uint32_t m_meshVertexCount;
    glm::vec3 m_sceneBoundsMin;
    glm::vec3 m_sceneBoundsMax;
    
    // Spatial hash
    std::vector<uint32_t> m_hashGrid;
    std::vector<uint32_t> m_hashOffsets;
    std::vector<uint32_t> m_hashCounts;
    
    // Helper functions
    bool createBuffers();
    bool createComputePipeline();
    bool createDescriptorSets();
    void dispatchVertexMerge(VkCommandBuffer cmd, uint32_t tileIndex);
    uint32_t computeTileCount(float thermalHeadroom);
    uint32_t computeHashCell(const glm::vec3& worldPos);
};
