#pragma once
#include "VulkanDevice.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <map>
#include <string>

namespace SecretEngine {
    class ICore;
}

struct Vertex3D {
    float position[3];
    float normal[3];
    float uv[2];
};

struct Vertex3DNitro {
    int16_t pos[4];    // 8 bytes - 16-bit Nitro Positioning (Supports wide range)
    int8_t  norm[4];   // 4 bytes - 8-bit Nitro Normals (Fast decoding)
    uint16_t uv[2];    // 4 bytes - 16-bit Nitro UVs (Texture fidelity)
};
static_assert(sizeof(Vertex3DNitro) == 16, "Vertex3DNitro must be 16 bytes for peak Nitrogen cache efficiency");

struct MeshHeader {
    char magic[4];
    uint32_t version;
    uint32_t vertexCount;
    uint32_t indexCount;
    float boundsMin[3];
    float boundsMax[3];
};

class Pipeline3D {
public:
    Pipeline3D();
    ~Pipeline3D();
    
    bool Initialize(VulkanDevice* device, VkRenderPass renderPass, SecretEngine::ICore* core);
    bool LoadMesh(const char* path);
    void Render(VkCommandBuffer cmd, float rotation, float aspect, const float camPos[3], const float camRot[3], const float* viewProj = nullptr);
    void Cleanup();
    
    void SetCubeColor(int colorIndex);
    void UpdateEntityPosition(int entityIndex, float x, float y);
    
private:
    bool CreatePipeline();
    
    VulkanDevice* m_device;
    SecretEngine::ICore* m_core;
    VkRenderPass m_renderPass;
    
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    
    struct MeshData {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexMemory;
        uint32_t vertexCount;
        uint32_t indexCount;
    };
    std::map<std::string, MeshData> m_meshes;
    
    // Instancing
    VkBuffer m_instanceBuffer;
    VkDeviceMemory m_instanceMemory;
    static constexpr int MAX_INSTANCES = 100;
    
    int m_currentColorIndex;
    
    // Multiple entities - removed local struct to use SecretEngine::Entity
};
