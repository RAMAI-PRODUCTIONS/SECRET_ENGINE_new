#pragma once
#include <cstdint>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {

// Forward+ inspired: Enhanced light data with attenuation parameters
struct LightData {
    enum Type { Directional = 0, Point = 1, Spot = 2 };
    
    Type type;
    float position[3];
    float direction[3];
    float color[3];
    float intensity;
    float range;
    float spotAngle;
    
    // Forward+ inspired: Attenuation parameters for realistic light falloff
    float constantAttenuation;   // Constant term (typically 1.0)
    float linearAttenuation;      // Linear term (distance-based)
    float quadraticAttenuation;   // Quadratic term (inverse square law)
    
    uint32_t padding[1];  // Alignment to 16-byte boundary
};

// Forward+ inspired: Tile-based culling configuration
struct TiledLightingConfig {
    uint32_t tileSize = 16;              // Tile size in pixels (16x16 recommended)
    uint32_t maxLightsPerTile = 256;     // Maximum lights per tile
    bool enableDebugVisualization = false;
    uint32_t debugMode = 0;              // 0=None, 1=DepthBuffer, 2=LightCount, 3=Frustums
};

// Forward+ inspired: Light culling statistics for profiling
struct LightCullingStats {
    uint32_t totalTiles = 0;
    uint32_t totalLights = 0;
    uint32_t averageLightsPerTile = 0;
    uint32_t maxLightsInTile = 0;
    float cullingTimeMs = 0.0f;
};

class ILightingSystem {
public:
    virtual ~ILightingSystem() = default;
    
    // Basic light management
    virtual uint32_t AddLight(const LightData& light) = 0;
    virtual void UpdateLight(uint32_t lightID, const LightData& light) = 0;
    virtual void RemoveLight(uint32_t lightID) = 0;
    virtual const LightData* GetLight(uint32_t lightID) const = 0;
    virtual uint32_t GetLightCount() const = 0;
    
    // C++26: Type-safe buffer access with std::span
    virtual std::span<const LightData> GetLightBuffer() const = 0;
    
    // Legacy API (deprecated, use GetLightBuffer() instead)
    virtual const void* GetLightBufferRaw() const = 0;
    virtual size_t GetLightBufferSize() const = 0;
    
    // Forward+ inspired: Tiled lighting configuration
    virtual void SetTiledLightingConfig(const TiledLightingConfig& config) = 0;
    virtual const TiledLightingConfig& GetTiledLightingConfig() const = 0;
    
    // Forward+ inspired: Get culling statistics for profiling
    virtual const LightCullingStats& GetCullingStats() const = 0;
    
    // Forward+ inspired: Enable/disable tiled forward rendering
    virtual void SetTiledRenderingEnabled(bool enabled) = 0;
    virtual bool IsTiledRenderingEnabled() const = 0;

    // FDA stream for zero-copy light updates (id=4 via GetInterface)
    // Returns Fast::UltraRingBuffer<2048>* cast to void*
    virtual void* GetLightUpdateStream() { return nullptr; }
};

} // namespace SecretEngine
