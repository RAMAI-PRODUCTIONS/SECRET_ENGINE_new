#include <SecretEngine/IRenderer.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/Entity.h>
#include <SecretEngine/Fast/FastData.h>
#include <SecretEngine/ILightingSystem.h>
#include <SecretEngine/IMaterialSystem.h>
#include <SecretEngine/ITextureSystem.h>
#include <SecretEngine/IShadowSystem.h>
#include "VulkanDevice.h"
#include "Window.h"
#include "Swapchain.h"
#include "MegaGeometryRenderer.h"
#include "TextureManager.h"
#include "MeshRenderingSystem.h"

// Forward declaration
namespace SecretEngine { class CameraPlugin; }

class RendererPlugin : public SecretEngine::IRenderer {
public:
    const char* GetName() const override { return "VulkanRenderer"; }
    uint32_t GetVersion() const override { return 1; }

    void OnLoad(SecretEngine::ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    
    void* GetInterface(uint32_t id) override {
        if (id == 1) return (SecretEngine::IRenderer*)this;
        return nullptr;
    }

    // IRenderer implementation
    SecretEngine::Fast::UltraRingBuffer<1024>& GetCommandStream() override { return m_commandStream; }
    
    void Submit() override;
    void Present() override;
    void InitializeHardware(void* nativeWindow) override;
    
    void SetCubeColor(int colorIndex) override;
    void SetDebugInfo(int slot, const char* text) override;
    void GetStats(uint32_t& instances, uint32_t& triangles, uint32_t& drawCalls) override;
    uint64_t GetVRAMUsage() override;
    void ClearAllInstances() override;

    // Particle / instance API
    uint32_t SpawnInstance(const char* meshPath, float x, float y, float z,
                           float r, float g, float b, float scale = 1.0f) override;
    void UpdateInstancePosColor(uint32_t id, float x, float y, float z,
                                float r, float g, float b, float scale = 1.0f) override;
    void DespawnInstance(uint32_t id, const char* meshPath) override;
    void DrawWelcomeText(VkCommandBuffer cmd);

private:
    void ProcessFastStream(); // NEW: Primary consumer for 8-byte packets
    void GetCameraMatrix(float* outViewProj); // Get camera from CameraPlugin

    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateSyncObjects();
    bool InitCommands();
    void CleanupSwapchain();
    bool RecreateSwapchain();
    
    // Triangle debugging
    bool CreateTrianglePipeline();
    void DrawTriangle(VkCommandBuffer cmd);
    
    // 2D rendering
    bool Create2DPipeline();
    bool Create2DVertexBuffer();

    std::string LoadAssetAsString(const char* filename);

    SecretEngine::ICore* m_core = nullptr;
    VulkanDevice* m_device = nullptr;
    Window* m_window = nullptr;
    Swapchain* m_swapchain = nullptr;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_framebuffers;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;
    int m_currentFrame = 0;
    
    VkPipeline m_trianglePipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_trianglePipelineLayout = VK_NULL_HANDLE;
    
    VkPipeline m_2dPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_2dPipelineLayout = VK_NULL_HANDLE;
    VkBuffer m_2dVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_2dVertexMemory = VK_NULL_HANDLE;
    uint32_t m_2dVertexCount = 0;
    
    float m_rotation = 0.0f;
    
    SecretEngine::Entity m_playerStartEntity = {0, 0};
    uint32_t m_skyVertexCount = 0;

    // Fast Data Protocol Stream (1024 commands buffer)
    SecretEngine::Fast::UltraRingBuffer<1024> m_commandStream;

    // Mega Geometry System + Texture System
    SecretEngine::MegaGeometry::MegaGeometryRenderer* m_megaGeometry = nullptr;
    SecretEngine::Textures::TextureManager* m_textureManager = nullptr;
    SecretEngine::MeshRenderingSystem* m_meshRenderingSystem = nullptr;
    
    // Plugin Systems (Modular Architecture)
    SecretEngine::ILightingSystem* m_lightingSystem = nullptr;
    SecretEngine::IMaterialSystem* m_materialSystem = nullptr;
    SecretEngine::ITextureSystem* m_textureSystemPlugin = nullptr;
    SecretEngine::IShadowSystem* m_shadowSystem = nullptr;
    
    // GPU Buffers for Plugin Data
    VkBuffer m_lightBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_lightMemory = VK_NULL_HANDLE;
    VkBuffer m_materialBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_materialMemory = VK_NULL_HANDLE;
    VkDescriptorSet m_lightDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet m_materialDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_lightDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_materialDescriptorLayout = VK_NULL_HANDLE;
    
    // Helper methods for plugin integration
    bool CreatePluginBuffers();
    void UpdateLightBuffer();
    void UpdateMaterialBuffer();
    void RenderShadowPass(VkCommandBuffer cmd);
    void DispatchVolumetricLighting(VkCommandBuffer cmd);
    
    // Stats
    int m_fps = 0;
    int m_frameCount = 0;
    float m_lastTime = 0.0f;
    
    struct Float3 { float x, y, z; };
    std::vector<Float3> m_instancePositions;
    std::vector<float> m_instanceRotationSpeeds;
    std::vector<Float3> m_instanceColors;
    float m_totalTime = 0.0f;
    std::map<int, std::string> m_debugStrings;
    
    // GPU Timing
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
    bool m_timestampsSupported = false;
    float m_timestampPeriod = 1.0f;
    bool CreateTimestampQueries();
    void RecordGPUTimestamps(VkCommandBuffer cmd);
    void ReadGPUTimestamps();
};
