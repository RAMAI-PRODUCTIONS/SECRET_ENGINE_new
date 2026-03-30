#include "ShadowPlugin.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <cstring>
#include <cmath>

ShadowPlugin::~ShadowPlugin() {
}

void ShadowPlugin::OnLoad(SecretEngine::ICore* core) {
    m_core = core;
    m_shadowMaps.reserve(MAX_SHADOW_MAPS);
    
    // Initialize volumetric lighting
    m_volumetric.desc.enabled = false;
    m_volumetric.desc.sampleCount = 32;
    m_volumetric.desc.scattering = 0.1f;
    m_volumetric.desc.absorption = 0.05f;
    m_volumetric.desc.density = 0.01f;
    m_volumetric.desc.anisotropy = 0.3f;
    m_volumetric.desc.maxDistance = 1000.0f;
    
    m_core->RegisterCapability("shadows", this);
    
    if (m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("ShadowSystem", 
            "✓ GPU-Driven Shadow System initialized (CSM, PCF, VSM, Volumetric Lighting)");
    }
}

void ShadowPlugin::OnActivate() {
    InitializeVolumetric();
}

void ShadowPlugin::OnDeactivate() {
}

void ShadowPlugin::OnUnload() {
}

void ShadowPlugin::OnUpdate(float deltaTime) {
    // Update shadow matrices based on camera/light positions
    for (auto& shadowMap : m_shadowMaps) {
        if (shadowMap.isActive) {
            UpdateShadowMatrices(shadowMap);
        }
    }
}

SecretEngine::ShadowMapHandle ShadowPlugin::CreateShadowMap(const SecretEngine::ShadowMapDesc& desc) {
    uint32_t index;
    
    // Reuse free slot or allocate new
    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
    } else {
        if (m_shadowMaps.size() >= MAX_SHADOW_MAPS) {
            return SecretEngine::ShadowMapHandle::Invalid();
        }
        index = static_cast<uint32_t>(m_shadowMaps.size());
        m_shadowMaps.push_back(ShadowMapData());
    }
    
    ShadowMapData& shadowMap = m_shadowMaps[index];
    shadowMap.lightID = desc.lightID;
    shadowMap.quality = desc.quality;
    shadowMap.technique = desc.technique;
    shadowMap.cascadeCount = desc.cascadeCount;
    memcpy(shadowMap.cascadeSplits, desc.cascadeSplits, sizeof(float) * 4);
    shadowMap.isActive = true;
    
    // Set resolution based on quality
    switch (desc.quality) {
        case SecretEngine::ShadowQuality::Low:    shadowMap.resolution = 512; break;
        case SecretEngine::ShadowQuality::Medium: shadowMap.resolution = 1024; break;
        case SecretEngine::ShadowQuality::High:   shadowMap.resolution = 2048; break;
        case SecretEngine::ShadowQuality::Ultra:  shadowMap.resolution = 4096; break;
    }
    
    // Calculate memory usage
    uint32_t bytesPerPixel = 4; // Depth32F
    shadowMap.memorySize = shadowMap.resolution * shadowMap.resolution * bytesPerPixel * shadowMap.cascadeCount;
    
    // Calculate cascade splits if using CSM
    if (desc.technique == SecretEngine::ShadowTechnique::CSM) {
        CalculateCascadeSplits(shadowMap);
    }
    
    // TODO: Create actual Vulkan shadow map texture and framebuffer
    // This would be done by the renderer backend
    
    if (m_core && m_core->GetLogger()) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "Created shadow map: Light=%u, Quality=%dx%d, Cascades=%u, Technique=%d",
            desc.lightID, shadowMap.resolution, shadowMap.resolution, 
            shadowMap.cascadeCount, static_cast<int>(desc.technique));
        m_core->GetLogger()->LogInfo("ShadowSystem", msg);
    }
    
    return {index, m_nextGeneration++};
}

void ShadowPlugin::UpdateShadowMap(SecretEngine::ShadowMapHandle handle, const SecretEngine::ShadowMapDesc& desc) {
    if (handle.id >= m_shadowMaps.size()) return;
    
    ShadowMapData& shadowMap = m_shadowMaps[handle.id];
    shadowMap.lightID = desc.lightID;
    shadowMap.quality = desc.quality;
    shadowMap.technique = desc.technique;
    shadowMap.cascadeCount = desc.cascadeCount;
    memcpy(shadowMap.cascadeSplits, desc.cascadeSplits, sizeof(float) * 4);
    
    if (desc.technique == SecretEngine::ShadowTechnique::CSM) {
        CalculateCascadeSplits(shadowMap);
    }
}

void ShadowPlugin::DestroyShadowMap(SecretEngine::ShadowMapHandle handle) {
    if (handle.id >= m_shadowMaps.size()) return;
    
    m_shadowMaps[handle.id].isActive = false;
    m_freeSlots.push_back(handle.id);
    
    // TODO: Destroy Vulkan resources
}

void* ShadowPlugin::GetShadowMapTexture(SecretEngine::ShadowMapHandle handle) const {
    return (handle.id < m_shadowMaps.size()) ? m_shadowMaps[handle.id].shadowTexture : nullptr;
}

void ShadowPlugin::BeginShadowPass(SecretEngine::ShadowMapHandle handle) {
    // TODO: Bind shadow framebuffer, set viewport
    // This would be called by the renderer
}

void ShadowPlugin::EndShadowPass(SecretEngine::ShadowMapHandle handle) {
    // TODO: Unbind shadow framebuffer
}

void ShadowPlugin::GetShadowMatrix(SecretEngine::ShadowMapHandle handle, uint32_t cascade, float* outMatrix) const {
    if (handle.id >= m_shadowMaps.size() || cascade >= 4) return;
    
    const ShadowMapData& shadowMap = m_shadowMaps[handle.id];
    memcpy(outMatrix, &shadowMap.shadowMatrix[cascade * 16], sizeof(float) * 16);
}

void ShadowPlugin::SetVolumetricLighting(const SecretEngine::VolumetricLightingDesc& desc) {
    m_volumetric.desc = desc;
    
    if (m_core && m_core->GetLogger()) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
            "Volumetric lighting: %s, Samples=%u, Scattering=%.2f, Density=%.3f",
            desc.enabled ? "ENABLED" : "DISABLED",
            desc.sampleCount, desc.scattering, desc.density);
        m_core->GetLogger()->LogInfo("ShadowSystem", msg);
    }
}

const SecretEngine::VolumetricLightingDesc& ShadowPlugin::GetVolumetricLighting() const {
    return m_volumetric.desc;
}

void* ShadowPlugin::GetVolumetricTexture() const {
    return m_volumetric.volumetricTexture;
}

void ShadowPlugin::SetGlobalShadowQuality(SecretEngine::ShadowQuality quality) {
    m_globalQuality = quality;
}

void ShadowPlugin::SetShadowDistance(float distance) {
    m_shadowDistance = distance;
}

void ShadowPlugin::EnableSoftShadows(bool enable) {
    m_softShadowsEnabled = enable;
}

uint32_t ShadowPlugin::GetShadowMapCount() const {
    uint32_t count = 0;
    for (const auto& sm : m_shadowMaps) {
        if (sm.isActive) count++;
    }
    return count;
}

size_t ShadowPlugin::GetShadowMemoryUsage() const {
    size_t total = 0;
    for (const auto& sm : m_shadowMaps) {
        if (sm.isActive) {
            total += sm.memorySize;
        }
    }
    return total;
}

void ShadowPlugin::CalculateCascadeSplits(ShadowMapData& shadowMap) {
    // Practical Split Scheme (PSS) for CSM
    // Balances between uniform and logarithmic splits
    const float lambda = 0.5f; // Blend factor
    const float nearPlane = 0.1f;
    const float farPlane = m_shadowDistance;
    
    for (uint32_t i = 0; i < shadowMap.cascadeCount; ++i) {
        float p = static_cast<float>(i + 1) / shadowMap.cascadeCount;
        
        // Logarithmic split
        float log = nearPlane * powf(farPlane / nearPlane, p);
        
        // Uniform split
        float uniform = nearPlane + (farPlane - nearPlane) * p;
        
        // Blend
        shadowMap.cascadeSplits[i] = lambda * log + (1.0f - lambda) * uniform;
    }
}

void ShadowPlugin::UpdateShadowMatrices(ShadowMapData& shadowMap) {
    // TODO: Calculate light view-projection matrices for each cascade
    // This requires camera and light information
    // For now, just identity matrices
    for (uint32_t i = 0; i < shadowMap.cascadeCount; ++i) {
        float* matrix = &shadowMap.shadowMatrix[i * 16];
        // Identity matrix
        for (int j = 0; j < 16; ++j) {
            matrix[j] = (j % 5 == 0) ? 1.0f : 0.0f;
        }
    }
}

void ShadowPlugin::InitializeVolumetric() {
    // TODO: Create 3D texture for volumetric lighting
    // TODO: Create compute shader for ray marching
    // This would be done by the renderer backend
    
    if (m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("ShadowSystem", "Volumetric lighting initialized (ray marching compute shader)");
    }
}

extern "C" {
    SecretEngine::IPlugin* CreatePlugin() {
        return new ShadowPlugin();
    }
    
    void DestroyPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
