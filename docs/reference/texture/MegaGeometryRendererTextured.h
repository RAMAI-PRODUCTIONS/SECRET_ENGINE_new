// ============================================================================
// GPU-DRIVEN MEGA-GEOMETRY RENDERER WITH TEXTURE SUPPORT
// Process 10M+ textured triangles at 60fps
// ============================================================================

#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <atomic>
#include <cstring>
#include <SecretEngine/Math.h>
#include <SecretEngine/IAssetProvider.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/Fast/FastData.h>
#include "TextureManager.h"

class VulkanDevice;

using SecretEngine::Math::InstanceData;

namespace SecretEngine::MegaGeometry {

// ============================================================================
// EXTENDED INSTANCE DATA WITH TEXTURE
// ============================================================================
struct InstanceDataTextured {
    SecretEngine::Math::Matrix3x4 transform;  // 48 bytes
    uint32_t packedColor;                     // 4 bytes
    uint32_t textureID;                       // 4 bytes - bindless texture index
    uint32_t _padding0;                       // 4 bytes
    uint32_t _padding1;                       // 4 bytes
};
static_assert(sizeof(InstanceDataTextured) == 64, "InstanceDataTextured must be 64 bytes");

// ============================================================================
// MEGA GEOMETRY RENDERER WITH TEXTURE SUPPORT
// ============================================================================
class MegaGeometryRendererTextured {
private:
    // === GPU BUFFERS (All persistent-mapped) ===
    VkBuffer m_instanceSSBO[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_instanceSSBOMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    InstanceDataTextured* m_instanceDataMapped[2] = {nullptr, nullptr};
    static constexpr uint32_t MAX_INSTANCES = 65536;
    
    VkBuffer m_indirectBuffer[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_indirectMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDrawIndexedIndirectCommand* m_indirectMapped[2] = {nullptr, nullptr};
    static constexpr uint32_t MAX_MESHES = 256;
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    void* m_vertexDataMapped = nullptr;
    uint32_t m_vertexOffset = 0; 
    static constexpr uint32_t MAX_VERTICES = 1000000;
    
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
    void* m_indexDataMapped = nullptr;
    uint32_t m_indexOffset = 0;
    static constexpr uint32_t MAX_INDICES = 3000000;
    
    // Pipeline with texture support
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    
    // Compute culling
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
    
    float m_viewProj[16];
    bool m_batchUpdateActive = false;
    
    // Texture manager
    Textures::TextureManager* m_textureManager = nullptr;
    
public:
    bool Initialize(VulkanDevice* device, VkRenderPass renderPass, SecretEngine::ICore* core, 
                   Textures::TextureManager* textureManager);
    
    // === INSTANCE MANAGEMENT ===
    void UpdateInstanceTransform(uint32_t instanceID, float x, float y, float z, 
                                float rotY = 0.0f, float rotX = 0.0f, float rotZ = 0.0f);
    void UpdateInstanceColor(uint32_t instanceID, float r, float g, float b, float a);
    void UpdateInstanceTexture(uint32_t instanceID, uint32_t textureID);
    
    // === RENDERING ===
    void SetViewProjection(const float* vp) {
        memcpy(m_viewProj, vp, sizeof(float) * 16);
    }
    
    void PreRender(VkCommandBuffer cmd);
    void Render(VkCommandBuffer cmd);
    
    // === BATCH UPDATES ===
    void BeginBatchUpdate() { m_batchUpdateActive = true; }
    void EndBatchUpdate() { m_batchUpdateActive = false; }
    
    // === MESH LOADING ===
    bool LoadMesh(const char* path, uint32_t meshSlot);
    
    // === INSTANCE CREATION ===
    // Creates instance with optional texture
    uint32_t AddInstance(uint32_t meshSlot, float x, float y, float z, uint32_t textureID = UINT32_MAX);
    void RemoveInstance(uint32_t instanceID, uint32_t meshSlot);
    
    struct RenderStats {
        uint32_t totalInstances;
        uint32_t totalTriangles;
        uint32_t drawCalls;
    };
    
    RenderStats GetStats() const;
    
    // === TEXTURE UTILITIES ===
    // Load texture and assign to instance
    uint32_t LoadAndAssignTexture(const char* texturePath, uint32_t instanceID);

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
};

} // namespace SecretEngine::MegaGeometry
