#pragma once
#include <cstdint>
#include <cstddef>

namespace SecretEngine {

enum class ShadowQuality {
    Low = 0,      // 512x512
    Medium = 1,   // 1024x1024
    High = 2,     // 2048x2048
    Ultra = 3     // 4096x4096
};

enum class ShadowTechnique {
    Basic = 0,           // Simple shadow mapping
    PCF = 1,             // Percentage Closer Filtering
    VSM = 2,             // Variance Shadow Maps
    CSM = 3              // Cascaded Shadow Maps
};

struct ShadowMapDesc {
    uint32_t lightID;
    ShadowQuality quality;
    ShadowTechnique technique;
    uint32_t cascadeCount;  // For CSM (1-4)
    float cascadeSplits[4]; // Split distances
    float bias;
    float normalBias;
};

struct ShadowMapHandle {
    uint32_t id;
    uint32_t generation;
    
    bool IsValid() const { return id != 0; }
    static ShadowMapHandle Invalid() { return {0, 0}; }
};

struct VolumetricLightingDesc {
    bool enabled;
    uint32_t sampleCount;      // Ray march samples (16-64)
    float scattering;          // Light scattering coefficient
    float absorption;          // Light absorption coefficient
    float density;             // Fog/atmosphere density
    float anisotropy;          // Phase function anisotropy (-1 to 1)
    float maxDistance;         // Max ray march distance
};

class IShadowSystem {
public:
    virtual ~IShadowSystem() = default;
    
    // Shadow map management
    virtual ShadowMapHandle CreateShadowMap(const ShadowMapDesc& desc) = 0;
    virtual void UpdateShadowMap(ShadowMapHandle handle, const ShadowMapDesc& desc) = 0;
    virtual void DestroyShadowMap(ShadowMapHandle handle) = 0;
    virtual void* GetShadowMapTexture(ShadowMapHandle handle) const = 0;
    
    // Shadow rendering
    virtual void BeginShadowPass(ShadowMapHandle handle) = 0;
    virtual void EndShadowPass(ShadowMapHandle handle) = 0;
    virtual void GetShadowMatrix(ShadowMapHandle handle, uint32_t cascade, float* outMatrix) const = 0;
    
    // Volumetric lighting
    virtual void SetVolumetricLighting(const VolumetricLightingDesc& desc) = 0;
    virtual const VolumetricLightingDesc& GetVolumetricLighting() const = 0;
    virtual void* GetVolumetricTexture() const = 0;
    
    // Configuration
    virtual void SetGlobalShadowQuality(ShadowQuality quality) = 0;
    virtual void SetShadowDistance(float distance) = 0;
    virtual void EnableSoftShadows(bool enable) = 0;
    
    // Stats
    virtual uint32_t GetShadowMapCount() const = 0;
    virtual size_t GetShadowMemoryUsage() const = 0;
};

} // namespace SecretEngine
