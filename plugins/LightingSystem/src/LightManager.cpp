#include "LightManager.h"
#include <algorithm>
#include <cmath>

namespace SecretEngine {

LightManager::LightManager() {
    m_lights.reserve(MAX_LIGHTS);
    m_activeSlots.reserve(MAX_LIGHTS);
    m_spatialData.reserve(MAX_LIGHTS);
    
    // Forward+ inspired: Default configuration optimized for performance
    m_tiledConfig.tileSize = 16;
    m_tiledConfig.maxLightsPerTile = 256;
    m_tiledConfig.enableDebugVisualization = false;
    m_tiledConfig.debugMode = 0;
}

LightManager::~LightManager() {
}

uint32_t LightManager::AddLight(const LightData& light) {
    if (m_lights.size() >= MAX_LIGHTS) {
        return 0;  // Failed
    }
    
    uint32_t id = m_nextID++;
    size_t index = m_lights.size();
    
    m_lights.push_back(light);
    m_activeSlots.push_back(true);
    m_idToIndex[id] = index;
    
    // Forward+ inspired: Update spatial data for culling
    UpdateSpatialData();
    
    return id;
}

void LightManager::UpdateLight(uint32_t lightID, const LightData& light) {
    auto it = m_idToIndex.find(lightID);
    if (it != m_idToIndex.end() && m_activeSlots[it->second]) {
        m_lights[it->second] = light;
        
        // Forward+ inspired: Update spatial data
        UpdateSpatialData();
    }
}

void LightManager::RemoveLight(uint32_t lightID) {
    auto it = m_idToIndex.find(lightID);
    if (it != m_idToIndex.end()) {
        m_activeSlots[it->second] = false;
        m_idToIndex.erase(it);
        
        // Forward+ inspired: Update spatial data
        UpdateSpatialData();
    }
}

const LightData* LightManager::GetLight(uint32_t lightID) const {
    auto it = m_idToIndex.find(lightID);
    if (it != m_idToIndex.end() && m_activeSlots[it->second]) {
        return &m_lights[it->second];
    }
    return nullptr;
}

uint32_t LightManager::GetLightCount() const {
    return static_cast<uint32_t>(std::count(m_activeSlots.begin(), m_activeSlots.end(), true));
}

std::span<const LightData> LightManager::GetLightBuffer() const {
    // Debug: Log the actual buffer state
    // Note: This is called every frame, so we only log once
    static bool logged = false;
    if (!logged) {
        logged = true;
        // Size will be logged by caller
    }
    return std::span<const LightData>(m_lights.data(), m_lights.size());
}

const void* LightManager::GetLightBufferRaw() const {
    return m_lights.empty() ? nullptr : m_lights.data();
}

size_t LightManager::GetLightBufferSize() const {
    return m_lights.size() * sizeof(LightData);
}

// Forward+ inspired: Tiled lighting configuration
void LightManager::SetTiledLightingConfig(const TiledLightingConfig& config) {
    m_tiledConfig = config;
}

const TiledLightingConfig& LightManager::GetTiledLightingConfig() const {
    return m_tiledConfig;
}

const LightCullingStats& LightManager::GetCullingStats() const {
    return m_cullingStats;
}

void LightManager::SetTiledRenderingEnabled(bool enabled) {
    m_tiledRenderingEnabled = enabled;
}

bool LightManager::IsTiledRenderingEnabled() const {
    return m_tiledRenderingEnabled;
}

void LightManager::UpdateCullingStats(uint32_t totalTiles, uint32_t avgLights, uint32_t maxLights, float timeMs) {
    m_cullingStats.totalTiles = totalTiles;
    m_cullingStats.totalLights = GetLightCount();
    m_cullingStats.averageLightsPerTile = avgLights;
    m_cullingStats.maxLightsInTile = maxLights;
    m_cullingStats.cullingTimeMs = timeMs;
}

std::span<const LightSpatialData> LightManager::GetSpatialData() const {
    return std::span<const LightSpatialData>(m_spatialData.data(), m_spatialData.size());
}

// Forward+ inspired: Calculate light attenuation using physically-based formula
float LightManager::CalculateAttenuation(const LightData& light, float distance) {
    if (light.type == LightData::Directional) {
        return 1.0f;  // Directional lights don't attenuate
    }
    
    // Blinn-Phong attenuation formula: 1 / (constant + linear*d + quadratic*d^2)
    float attenuation = 1.0f / (
        light.constantAttenuation + 
        light.linearAttenuation * distance + 
        light.quadraticAttenuation * distance * distance
    );
    
    return attenuation * light.intensity;
}

// Forward+ inspired: Calculate effective radius where light contribution becomes negligible
float LightManager::GetEffectiveRadius(const LightData& light, float threshold) {
    if (light.type == LightData::Directional) {
        return std::numeric_limits<float>::infinity();
    }
    
    // Solve for distance where attenuation * intensity = threshold
    // threshold = intensity / (c + l*d + q*d^2)
    // Rearranging: q*d^2 + l*d + (c - intensity/threshold) = 0
    
    float a = light.quadraticAttenuation;
    float b = light.linearAttenuation;
    float c = light.constantAttenuation - (light.intensity / threshold);
    
    // Quadratic formula
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return light.range;  // Fallback to specified range
    }
    
    float radius = (-b + std::sqrt(discriminant)) / (2.0f * a);
    
    // Clamp to specified range
    return std::min(radius, light.range);
}

// Forward+ inspired: Update spatial data for efficient culling
void LightManager::UpdateSpatialData() {
    m_spatialData.clear();
    
    for (size_t i = 0; i < m_lights.size(); ++i) {
        if (!m_activeSlots[i]) continue;
        
        const LightData& light = m_lights[i];
        
        LightSpatialData spatial;
        spatial.position[0] = light.position[0];
        spatial.position[1] = light.position[1];
        spatial.position[2] = light.position[2];
        spatial.lightIndex = static_cast<uint32_t>(i);
        
        // Calculate effective bounding sphere radius for culling
        spatial.boundingSphereRadius = GetEffectiveRadius(light);
        
        m_spatialData.push_back(spatial);
    }
}

} // namespace SecretEngine
