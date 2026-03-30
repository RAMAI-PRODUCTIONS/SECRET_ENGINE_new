#pragma once
#include <SecretEngine/ILightingSystem.h>
#include <vector>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {

class LightManager {
public:
    LightManager();
    ~LightManager();
    
    uint32_t AddLight(const LightData& light);
    void UpdateLight(uint32_t lightID, const LightData& light);
    void RemoveLight(uint32_t lightID);
    const LightData* GetLight(uint32_t lightID) const;
    uint32_t GetLightCount() const;
    
    // C++26: Type-safe buffer access
    std::span<const LightData> GetLightBuffer() const;
    
    // Legacy API
    const void* GetLightBufferRaw() const;
    size_t GetLightBufferSize() const;
    
private:
    static constexpr uint32_t MAX_LIGHTS = 256;
    std::vector<LightData> m_lights;
    std::vector<bool> m_activeSlots;
    uint32_t m_nextID = 1;
};

} // namespace SecretEngine
