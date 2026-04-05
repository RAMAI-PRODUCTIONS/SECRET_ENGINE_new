// ============================================================================
// GPU-DRIVEN MEGA-GEOMETRY RENDERER
// Process 10M+ triangles at 60fps using FDA + Vulkan Indirect Draw
// ============================================================================

#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <cstring>
#include <unordered_map>
#include <string>
#include <SecretEngine/Math.h>
#include <SecretEngine/IAssetProvider.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/Fast/FastData.h>
#include "TextureManager.h"

class VulkanDevice;

// Instance data structure is now in SecretEngine/Math.h
using SecretEngine::Math::InstanceData;

namespace SecretEngine::Textures { class TextureManager; }

// ============================================================================
// INDIRECT DRAW COMMAND (Vulkan Standard Format)
// ============================================================================

namespace SecretEngine::MegaGeometry {

// ============================================================================
// MEGA GEOMETRY SYSTEM
// Handles millions of triangles using GPU-driven rendering
// ============================================================================
class MegaGeometryRenderer {
private:
    
    // === GPU BUFFERS (All persistent-mapped) ===
    
    // Instance data (65,536 instances max) - DOUBLE BUFFERED
    VkBuffer m_instanceSSBO[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_instanceSSBOMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    InstanceData* m_instanceDataMapped[2] = {nullptr, nullptr}; // PERSISTENT MAP!
    static constexpr uint32_t MAX_INSTANCES = 65536;
    
    // Indirect draw commands (one per mesh type) - DOUBLE BUFFERED
    VkBuffer m_indirectBuffer[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_indirectMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDrawIndexedIndirectCommand* m_indirectMapped[2] = {nullptr, nullptr};
    static constexpr uint32_t MAX_MESHES = 256;
    
    // Shared vertex buffer (One huge buffer for all meshes)
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    void* m_vertexDataMapped = nullptr; // PERSISTENT MAP
    uint32_t m_vertexOffset = 0; 
    static constexpr uint32_t MAX_VERTICES = 1000000;
    
    // Shared index buffer
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
    void* m_indexDataMapped = nullptr; // PERSISTENT MAP
    uint32_t m_indexOffset = 0;
    static constexpr uint32_t MAX_INDICES = 3000000;
    
    // Pipeline
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    
    // Virtual Geometry (Compute Culling)
    VkPipeline m_cullPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_cullLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_cullDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_cullDescriptorSet[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkBuffer m_visibleInstanceSSBO[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_visibleInstanceMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    
    uint32_t m_frameIndex = 0;
    
    // Stats
    std::atomic<uint32_t> m_totalInstances{0};
    std::atomic<uint32_t> m_totalTriangles{0};
    
    // Camera
    float m_viewProj[16];
    
    // Dynamic lighting
    VkBuffer m_lightBuffer = VK_NULL_HANDLE;
    uint32_t m_lightCount = 0;
    VkDescriptorSetLayout m_lightDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_lightDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool m_lightDescriptorPool = VK_NULL_HANDLE;
    bool m_lightDescriptorDirty = false;
    
    // Batch update flag
    bool m_batchUpdateActive = false;
    
public:
    bool Initialize(VulkanDevice* device, VkRenderPass renderPass, SecretEngine::ICore* core, SecretEngine::Textures::TextureManager* textureManager);
    
    void ProcessPacket(const SecretEngine::Fast::UltraPacket& packet) {
        switch (packet.GetType()) {
            case SecretEngine::Fast::PacketType::RenderTransform: {
                uint32_t instanceID = packet.GetMetadata();
                if (instanceID < MAX_INSTANCES) {
                    float x = static_cast<float>(packet.dataA) / 32767.0f;
                    float y = static_cast<float>(packet.dataB) / 32767.0f;
                    UpdateInstanceTransform(instanceID, x, y, 0.0f);
                }
                break;
            }
            case SecretEngine::Fast::PacketType::RenderCommand: {
                uint32_t instanceID = packet.GetMetadata();
                if (instanceID < MAX_INSTANCES) {
                    // CMD ID 1: Update Color (dataA = color index or packed)
                    UpdateInstanceColor(instanceID, 1.0f, 1.0f, 1.0f, 1.0f); 
                }
                break;
            }
            default:
                break;
        }
    }
    
    void UpdateInstanceTransform(uint32_t instanceID, float x, float y, float z, float rotY = 0.0f, float rotX = 0.0f, float rotZ = 0.0f);
    void UpdateInstanceColor(uint32_t instanceID, float r, float g, float b, float a);
    void UpdateInstanceTexture(uint32_t instanceID, uint32_t textureID);
    
    void SetViewProjection(const float* vp) {
        memcpy(m_viewProj, vp, sizeof(float) * 16);
    }
    
    // Dynamic lighting support
    void SetLightBuffer(VkBuffer buffer, VkDeviceSize size, uint32_t count);
    
    void PreRender(VkCommandBuffer cmd);
    void Render(VkCommandBuffer cmd);
    
    // Batch update optimization
    void BeginBatchUpdate() { m_batchUpdateActive = true; }
    void EndBatchUpdate() { m_batchUpdateActive = false; }
    
    bool LoadMesh(const char* path, uint32_t meshSlot);
    uint32_t GetOrLoadMeshSlot(const char* meshPath); // Get existing slot or load mesh into new slot
    uint32_t AddInstance(uint32_t meshSlot, float x, float y, float z, uint32_t textureID = UINT32_MAX);
    void RemoveInstance(uint32_t instanceID, uint32_t meshSlot);
    void ClearAllInstances(); // Clear all instances when changing levels
    
    struct RenderStats {
        uint32_t totalInstances;
        uint32_t totalTriangles;
        uint32_t drawCalls;
    };
    
    RenderStats GetStats() const;

private:
    bool CreateGeometricBuffers();
    bool CreateInstanceSSBO();
    bool CreateIndirectBuffer();
    bool CreateDescriptorSet();
    bool CreatePipeline(VkRenderPass renderPass);
    bool CreateCullPipeline();
    bool CreateVisibleBuffer();
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VulkanDevice* m_device = nullptr;
    SecretEngine::ICore* m_core = nullptr;
    SecretEngine::Textures::TextureManager* m_textureManager = nullptr;
    
    // Mesh path to slot mapping
    std::unordered_map<std::string, uint32_t> m_meshPathToSlot;
    uint32_t m_nextMeshSlot = 0;
};

} // namespace SecretEngine::MegaGeometry
