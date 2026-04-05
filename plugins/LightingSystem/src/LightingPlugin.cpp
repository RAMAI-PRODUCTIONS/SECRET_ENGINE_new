#include "LightingPlugin.h"
#include "LightManager.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>

LightingPlugin::~LightingPlugin() {
    if (m_lightManager) {
        delete m_lightManager;
        m_lightManager = nullptr;
    }
}

void LightingPlugin::OnLoad(SecretEngine::ICore* core) {
    m_core = core;
    m_lightManager = new SecretEngine::LightManager();
    
    // Register as lighting system capability
    m_core->RegisterCapability("lighting", this);
    
    // Store 'this' pointer for verification
    m_selfPointer = this;
    
    if (m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin loaded");
        
        // Forward+ inspired: Log configuration
        const auto& config = m_lightManager->GetTiledLightingConfig();
        char buffer[256];
        snprintf(buffer, sizeof(buffer), 
                "Forward+ Configuration: TileSize=%u, MaxLightsPerTile=%u, MaxTotalLights=1024",
                config.tileSize, config.maxLightsPerTile);
        m_core->GetLogger()->LogInfo("LightingSystem", buffer);
    }
}

void LightingPlugin::OnActivate() {
    if (m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin activated");
    }
}

void LightingPlugin::OnDeactivate() {
    if (m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin deactivated");
    }
}

void LightingPlugin::OnUnload() {
    // Only log once - if we're seeing this multiple times, something is wrong
    static bool unloaded = false;
    if (!unloaded) {
        unloaded = true;
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogInfo("LightingSystem", "Plugin unloaded - THIS SHOULD ONLY APPEAR ONCE!");
        }
    }
    // Clear the light manager to prevent dangling pointers
    if (m_lightManager) {
        delete m_lightManager;
        m_lightManager = nullptr;
    }
}

void LightingPlugin::OnUpdate(float deltaTime) {
    // Forward+ inspired: Log culling statistics periodically
    static float statsTimer = 0.0f;
    statsTimer += deltaTime;
    
    if (statsTimer >= 5.0f && m_lightManager && m_core && m_core->GetLogger()) {
        statsTimer = 0.0f;
        
        if (m_lightManager->IsTiledRenderingEnabled()) {
            const auto& stats = m_lightManager->GetCullingStats();
            char buffer[512];
            snprintf(buffer, sizeof(buffer),
                    "Forward+ Stats: Lights=%u, Tiles=%u, AvgLightsPerTile=%u, MaxLightsInTile=%u, CullingTime=%.2fms",
                    stats.totalLights, stats.totalTiles, stats.averageLightsPerTile, 
                    stats.maxLightsInTile, stats.cullingTimeMs);
            m_core->GetLogger()->LogInfo("LightingSystem", buffer);
        }
    }
}

uint32_t LightingPlugin::AddLight(const SecretEngine::LightData& light) {
    return m_lightManager ? m_lightManager->AddLight(light) : 0;
}

void LightingPlugin::UpdateLight(uint32_t lightID, const SecretEngine::LightData& light) {
    if (m_lightManager) {
        m_lightManager->UpdateLight(lightID, light);
    }
}

void LightingPlugin::RemoveLight(uint32_t lightID) {
    if (m_lightManager) {
        m_lightManager->RemoveLight(lightID);
    }
}

const SecretEngine::LightData* LightingPlugin::GetLight(uint32_t lightID) const {
    return m_lightManager ? m_lightManager->GetLight(lightID) : nullptr;
}

uint32_t LightingPlugin::GetLightCount() const {
    return m_lightManager ? m_lightManager->GetLightCount() : 0;
}

std::span<const SecretEngine::LightData> LightingPlugin::GetLightBuffer() const {
    // Debug: Log internal state (only once)
    static bool logged = false;
    if (!logged && m_core && m_core->GetLogger()) {
        logged = true;
        char buf[256];
        snprintf(buf, sizeof(buf), "GetLightBuffer: this=%p, m_selfPointer=%p, m_lightManager=%p", 
                 (void*)this, m_selfPointer, (void*)m_lightManager);
        m_core->GetLogger()->LogInfo("LightingSystem", buf);
    }
    
    // Safety check: validate m_lightManager pointer
    if (!m_lightManager) {
        return std::span<const SecretEngine::LightData>{};
    }
    
    // Get the buffer from the manager
    auto buffer = m_lightManager->GetLightBuffer();
    
    // Sanity check: if the size is absurdly large, return empty span
    // Max lights is 1024, so any count above that is clearly corruption
    if (buffer.size() > 1024) {
        return std::span<const SecretEngine::LightData>{};
    }
    
    return buffer;
}

const void* LightingPlugin::GetLightBufferRaw() const {
    return m_lightManager ? m_lightManager->GetLightBufferRaw() : nullptr;
}

size_t LightingPlugin::GetLightBufferSize() const {
    return m_lightManager ? m_lightManager->GetLightBufferSize() : 0;
}

// Forward+ inspired: Tiled lighting configuration
void LightingPlugin::SetTiledLightingConfig(const SecretEngine::TiledLightingConfig& config) {
    if (m_lightManager) {
        m_lightManager->SetTiledLightingConfig(config);
        
        if (m_core && m_core->GetLogger()) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                    "Tiled lighting config updated: TileSize=%u, MaxLightsPerTile=%u",
                    config.tileSize, config.maxLightsPerTile);
            m_core->GetLogger()->LogInfo("LightingSystem", buffer);
        }
    }
}

const SecretEngine::TiledLightingConfig& LightingPlugin::GetTiledLightingConfig() const {
    static SecretEngine::TiledLightingConfig defaultConfig;
    return m_lightManager ? m_lightManager->GetTiledLightingConfig() : defaultConfig;
}

const SecretEngine::LightCullingStats& LightingPlugin::GetCullingStats() const {
    static SecretEngine::LightCullingStats defaultStats;
    return m_lightManager ? m_lightManager->GetCullingStats() : defaultStats;
}

void LightingPlugin::SetTiledRenderingEnabled(bool enabled) {
    if (m_lightManager) {
        m_lightManager->SetTiledRenderingEnabled(enabled);
        
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogInfo("LightingSystem", 
                enabled ? "Forward+ tiled rendering ENABLED" : "Forward+ tiled rendering DISABLED");
        }
    }
}

bool LightingPlugin::IsTiledRenderingEnabled() const {
    return m_lightManager ? m_lightManager->IsTiledRenderingEnabled() : false;
}

extern "C" {
    SecretEngine::IPlugin* CreatePlugin() {
        return new LightingPlugin();
    }
    
    SecretEngine::IPlugin* CreateLightingPlugin() {
        return new LightingPlugin();
    }
    
    void DestroyPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}

