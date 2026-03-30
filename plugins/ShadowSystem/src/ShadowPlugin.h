#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/IShadowSystem.h>
#include <vector>
#include <unordered_map>

namespace SecretEngine {
class ICore;
}

struct ShadowMapData {
    void* shadowTexture = nullptr;      // Depth texture
    void* framebuffer = nullptr;        // Shadow FBO
    uint32_t lightID = 0;
    SecretEngine::ShadowQuality quality;
    SecretEngine::ShadowTechnique technique;
    uint32_t cascadeCount = 1;
    float cascadeSplits[4] = {0};
    float shadowMatrix[16 * 4] = {0};   // Up to 4 cascades
    uint32_t resolution = 1024;
    size_t memorySize = 0;
    bool isActive = false;
};

struct VolumetricData {
    void* volumetricTexture = nullptr;  // 3D texture for volumetric lighting
    void* computePipeline = nullptr;    // Compute shader for ray marching
    SecretEngine::VolumetricLightingDesc desc;
};

// GPU-Driven Shadow & Volumetric Lighting System
// Features:
// - Cascaded Shadow Maps (CSM) for directional lights
// - PCF/VSM for soft shadows
// - Volumetric lighting (god rays, fog)
// - Zero CPU overhead (GPU compute)
class ShadowPlugin : public SecretEngine::IPlugin, public SecretEngine::IShadowSystem {
public:
    ShadowPlugin() = default;
    ~ShadowPlugin() override;
    
    // IPlugin
    const char* GetName() const override { return "ShadowSystem"; }
    uint32_t GetVersion() const override { return 1; }
    void OnLoad(SecretEngine::ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float deltaTime) override;
    
    // IShadowSystem
    SecretEngine::ShadowMapHandle CreateShadowMap(const SecretEngine::ShadowMapDesc& desc) override;
    void UpdateShadowMap(SecretEngine::ShadowMapHandle handle, const SecretEngine::ShadowMapDesc& desc) override;
    void DestroyShadowMap(SecretEngine::ShadowMapHandle handle) override;
    void* GetShadowMapTexture(SecretEngine::ShadowMapHandle handle) const override;
    
    void BeginShadowPass(SecretEngine::ShadowMapHandle handle) override;
    void EndShadowPass(SecretEngine::ShadowMapHandle handle) override;
    void GetShadowMatrix(SecretEngine::ShadowMapHandle handle, uint32_t cascade, float* outMatrix) const override;
    
    void SetVolumetricLighting(const SecretEngine::VolumetricLightingDesc& desc) override;
    const SecretEngine::VolumetricLightingDesc& GetVolumetricLighting() const override;
    void* GetVolumetricTexture() const override;
    
    void SetGlobalShadowQuality(SecretEngine::ShadowQuality quality) override;
    void SetShadowDistance(float distance) override;
    void EnableSoftShadows(bool enable) override;
    
    uint32_t GetShadowMapCount() const override;
    size_t GetShadowMemoryUsage() const override;
    
private:
    void CalculateCascadeSplits(ShadowMapData& shadowMap);
    void UpdateShadowMatrices(ShadowMapData& shadowMap);
    void InitializeVolumetric();
    
    SecretEngine::ICore* m_core = nullptr;
    std::vector<ShadowMapData> m_shadowMaps;
    std::vector<uint32_t> m_freeSlots;
    uint32_t m_nextGeneration = 1;
    
    VolumetricData m_volumetric;
    SecretEngine::ShadowQuality m_globalQuality = SecretEngine::ShadowQuality::Medium;
    float m_shadowDistance = 100.0f;
    bool m_softShadowsEnabled = true;
    
    static constexpr uint32_t MAX_SHADOW_MAPS = 32;
};

extern "C" {
#ifdef _WIN32
    __declspec(dllexport) SecretEngine::IPlugin* CreatePlugin();
    __declspec(dllexport) void DestroyPlugin(SecretEngine::IPlugin* plugin);
#else
    SecretEngine::IPlugin* CreatePlugin();
    void DestroyPlugin(SecretEngine::IPlugin* plugin);
#endif
}
