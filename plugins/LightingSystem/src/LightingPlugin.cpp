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
    
    if (m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin loaded");
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
    if (m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin unloaded");
    }
}

void LightingPlugin::OnUpdate(float deltaTime) {
    // Update light animations, shadows, etc.
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
    return m_lightManager ? m_lightManager->GetLightBuffer() : std::span<const SecretEngine::LightData>{};
}

const void* LightingPlugin::GetLightBufferRaw() const {
    return m_lightManager ? m_lightManager->GetLightBufferRaw() : nullptr;
}

size_t LightingPlugin::GetLightBufferSize() const {
    return m_lightManager ? m_lightManager->GetLightBufferSize() : 0;
}

extern "C" {
    SecretEngine::IPlugin* CreatePlugin() {
        return new LightingPlugin();
    }
    
    void DestroyPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
