#pragma once
#include <cstdint>
#include <cstddef>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {

struct MaterialProperties {
    float baseColor[4];
    float metallic;
    float roughness;
    float emissive[3];
    float emissiveStrength;
    uint32_t albedoTexture;
    uint32_t normalTexture;
    uint32_t metallicRoughnessTexture;
    uint32_t flags;
    float padding[3];  // Alignment
};

struct MaterialHandle {
    uint32_t id;
    uint32_t generation;
};

class IMaterialSystem {
public:
    virtual ~IMaterialSystem() = default;
    
    virtual MaterialHandle CreateMaterial(const char* name, const MaterialProperties& props) = 0;
    virtual void UpdateMaterial(MaterialHandle handle, const MaterialProperties& props) = 0;
    virtual void DestroyMaterial(MaterialHandle handle) = 0;
    virtual const MaterialProperties* GetMaterial(MaterialHandle handle) const = 0;
    virtual MaterialHandle GetMaterialByName(const char* name) const = 0;
    
    // C++26: Type-safe buffer access with std::span
    virtual std::span<const MaterialProperties> GetMaterialBuffer() const = 0;
    
    // Legacy API (deprecated, use GetMaterialBuffer() instead)
    virtual const void* GetMaterialBufferRaw() const = 0;
    virtual size_t GetMaterialBufferSize() const = 0;
    virtual uint32_t GetMaterialCount() const = 0;
};

} // namespace SecretEngine
