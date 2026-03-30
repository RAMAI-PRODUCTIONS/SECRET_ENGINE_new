#pragma once
#include <cstdint>

namespace SecretEngine {

struct LightData {
    enum Type { Directional = 0, Point = 1, Spot = 2 };
    
    Type type;
    float position[3];
    float direction[3];
    float color[3];
    float intensity;
    float range;
    float spotAngle;
    uint32_t padding[2];  // Alignment
};

class ILightingSystem {
public:
    virtual ~ILightingSystem() = default;
    
    virtual uint32_t AddLight(const LightData& light) = 0;
    virtual void UpdateLight(uint32_t lightID, const LightData& light) = 0;
    virtual void RemoveLight(uint32_t lightID) = 0;
    virtual const LightData* GetLight(uint32_t lightID) const = 0;
    virtual uint32_t GetLightCount() const = 0;
    virtual const void* GetLightBuffer() const = 0;
    virtual size_t GetLightBufferSize() const = 0;
};

} // namespace SecretEngine
