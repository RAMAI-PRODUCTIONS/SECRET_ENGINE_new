#pragma once
#include <SecretEngine/ILightingSystem.h>
#include <vector>
#include <unordered_map>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {

// Forward+ inspired: Spatial data structure for efficient light queries
struct LightSpatialData {
    float boundingSphereRadius;
    float position[3];
    uint32_t lightIndex;
};

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
    
    // Forward+ inspired: Tiled lighting configuration
    void SetTiledLightingConfig(const TiledLightingConfig& config);
    const TiledLightingConfig& GetTiledLightingConfig() const;
    
    // Forward+ inspired: Get culling statistics
    const LightCullingStats& GetCullingStats() const;
    
    // Forward+ inspired: Tiled rendering control
    void SetTiledRenderingEnabled(bool enabled);
    bool IsTiledRenderingEnabled() const;
    
    // Forward+ inspired: Update culling statistics (called by renderer)
    void UpdateCullingStats(uint32_t totalTiles, uint32_t avgLights, uint32_t maxLights, float timeMs);
    
    // Forward+ inspired: Get spatial data for frustum culling
    std::span<const LightSpatialData> GetSpatialData() const;
    
    // Forward+ inspired: Calculate light attenuation at distance
    static float CalculateAttenuation(const LightData& light, float distance);
    
    // Forward+ inspired: Get effective light radius (where attenuation drops below threshold)
    static float GetEffectiveRadius(const LightData& light, float threshold = 0.01f);
    
private:
    // Forward+ inspired: Increased max lights for tiled rendering (1024 lights supported)
    static constexpr uint32_t MAX_LIGHTS = 1024;
    
    std::vector<LightData> m_lights;
    std::vector<bool> m_activeSlots;
    std::unordered_map<uint32_t, size_t> m_idToIndex;  // O(1) lookup
    uint32_t m_nextID = 1;
    
    // Forward+ inspired: Spatial data for culling
    std::vector<LightSpatialData> m_spatialData;
    
    // Forward+ inspired: Configuration and statistics
    TiledLightingConfig m_tiledConfig;
    LightCullingStats m_cullingStats;
    bool m_tiledRenderingEnabled = false;
    
    // Forward+ inspired: Update spatial data when lights change
    void UpdateSpatialData();
};

} // namespace SecretEngine
