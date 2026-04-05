#pragma once
// ============================================================================
// MEGA LIGHT RENDERER
// Zero-copy, lock-free, persistent-mapped light SSBO system.
// Mirrors MegaGeometryRenderer architecture for lights.
// ============================================================================
#include <vulkan/vulkan.h>
#include <atomic>
#include <cstring>
#include <cmath>
#include <SecretEngine/Fast/FastData.h>
#include <SecretEngine/ILightingSystem.h>

class VulkanDevice;

namespace SecretEngine {

// GPU-side light struct (must match GLSL std430 layout in mega_geometry.frag)
// 68 bytes, individual floats — no vec3 alignment issues
struct alignas(4) GPULightData {
    int   type;
    float posX, posY, posZ;
    float dirX, dirY, dirZ;
    float colR, colG, colB;
    float intensity;
    float range;
    float spotAngle;
    float constAtten;
    float linearAtten;
    float quadAtten;
    uint32_t padding;
    // Total: 68 bytes
};
static_assert(sizeof(GPULightData) == 68, "GPULightData must be 68 bytes");

class MegaLightRenderer {
public:
    static constexpr uint32_t MAX_LIGHTS = 1024;
    static constexpr uint32_t MAX_VISIBLE = 32; // lights passed to shader per frame

    bool Initialize(VulkanDevice* device);
    bool InitWithHandles(VkDevice dev, VkPhysicalDevice phys);
    void Shutdown();

    // Called by LightingPlugin::AddLight — writes directly to persistent SSBO
    uint32_t AddLight(const LightData& light);

    // Lock-free: push FDA packets for position+colour update
    void PushLightUpdate(uint32_t idx, float x, float y, float z,
                         float r, float g, float b, float intensity);

    // Called by renderer each frame: drain FDA ring buffer → write to SSBO
    // Also sorts by distance and fills visible list
    void ProcessPackets(const float* cameraPos);

    // Renderer binds this SSBO as set=1
    VkBuffer GetLightSSBO() const { return m_lightSSBO[m_readBuf]; }
    uint32_t GetVisibleCount() const { return m_visibleCount.load(std::memory_order_relaxed); }
    uint32_t GetTotalCount() const { return m_lightCount.load(std::memory_order_relaxed); }

    // Expose FDA stream so FPSGamePlugin can push packets directly
    Fast::UltraRingBuffer<2048>& GetStream() { return m_stream; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VulkanDevice* m_vulkanDevice = nullptr;

    // Double-buffered persistent-mapped SSBO (like MegaGeometryRenderer)
    VkBuffer       m_lightSSBO[2]       = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_lightSSBOMemory[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    GPULightData*  m_mapped[2]          = {nullptr, nullptr};
    uint32_t       m_writeBuf           = 0;
    uint32_t       m_readBuf            = 1;

    std::atomic<uint32_t> m_lightCount{0};
    std::atomic<uint32_t> m_visibleCount{0};

    // FDA lock-free ring buffer for light updates
    Fast::UltraRingBuffer<2048> m_stream;

    // Visible light indices (sorted by distance each frame)
    uint32_t m_visibleIndices[MAX_VISIBLE] = {};

    void CopyToReadBuffer();
    void SortVisibleLights(const float* camPos);
};

} // namespace SecretEngine
