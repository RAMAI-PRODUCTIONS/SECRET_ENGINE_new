#include "LightingPlugin.h"
#include "LightManager.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>

LightingPlugin::~LightingPlugin() {
    if (m_lightManager) { delete m_lightManager; m_lightManager = nullptr; }
}

void LightingPlugin::OnLoad(SecretEngine::ICore* core) {
    m_core = core;
    m_lightManager = new SecretEngine::LightManager();
    m_core->RegisterCapability("lighting", this);
    if (m_core->GetLogger())
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin loaded");
}

void LightingPlugin::OnActivate() {
    if (m_core && m_core->GetLogger())
        m_core->GetLogger()->LogInfo("LightingSystem", "Plugin activated");
}

void LightingPlugin::OnDeactivate() {}

void LightingPlugin::OnUnload() {
    if (m_lightManager) { delete m_lightManager; m_lightManager = nullptr; }
}

void LightingPlugin::OnUpdate(float /*dt*/) {}

uint32_t LightingPlugin::AddLight(const SecretEngine::LightData& light) {
    uint32_t id = m_nextID++;
    if (m_lightManager) m_lightManager->AddLight(light);
    AllocSlot(id);
    return id;
}

void LightingPlugin::UpdateLight(uint32_t lightID, const SecretEngine::LightData& light) {
    if (m_lightManager) m_lightManager->UpdateLight(lightID, light);
    // Push FDA packet if stream is connected (set by RendererPlugin)
    if (m_fdaStream) {
        uint32_t slot = GetSlotForLight(lightID);
        if (slot == UINT32_MAX) return;
        // Packet 1: posX, posY
        { SecretEngine::Fast::UltraPacket p;
          p.Set(SecretEngine::Fast::PacketType::LightPos, slot,
                (int16_t)(light.position[0]*10.f), (int16_t)(light.position[1]*10.f));
          m_fdaStream->Push(p); }
        // Packet 2: posZ, colRG
        { SecretEngine::Fast::UltraPacket p;
          uint8_t pr=(uint8_t)(light.color[0]*255.f), pg=(uint8_t)(light.color[1]*255.f);
          p.Set(SecretEngine::Fast::PacketType::LightPosZColor, slot,
                (int16_t)(light.position[2]*10.f), (int16_t)(pr|(pg<<8)));
          m_fdaStream->Push(p); }
        // Packet 3: colB + intensity
        { SecretEngine::Fast::UltraPacket p;
          uint8_t pb=(uint8_t)(light.color[2]*255.f), pi=(uint8_t)(light.intensity*63.75f);
          p.Set(SecretEngine::Fast::PacketType::LightPosZColor, slot|(1u<<23),
                (int16_t)(pb|(pi<<8)), 0);
          m_fdaStream->Push(p); }
    }
}

void LightingPlugin::RemoveLight(uint32_t lightID) {
    if (m_lightManager) m_lightManager->RemoveLight(lightID);
    m_idToSlot.erase(lightID);
}

const SecretEngine::LightData* LightingPlugin::GetLight(uint32_t lightID) const {
    return m_lightManager ? m_lightManager->GetLight(lightID) : nullptr;
}

uint32_t LightingPlugin::GetLightCount() const {
    return m_lightManager ? m_lightManager->GetLightCount() : 0;
}

std::span<const SecretEngine::LightData> LightingPlugin::GetLightBuffer() const {
    return m_lightManager ? m_lightManager->GetLightBuffer()
                          : std::span<const SecretEngine::LightData>{};
}

const void* LightingPlugin::GetLightBufferRaw() const {
    return m_lightManager ? m_lightManager->GetLightBufferRaw() : nullptr;
}

size_t LightingPlugin::GetLightBufferSize() const {
    return m_lightManager ? m_lightManager->GetLightBufferSize() : 0;
}

void LightingPlugin::SetTiledLightingConfig(const SecretEngine::TiledLightingConfig& c) {
    if (m_lightManager) m_lightManager->SetTiledLightingConfig(c);
}

const SecretEngine::TiledLightingConfig& LightingPlugin::GetTiledLightingConfig() const {
    static SecretEngine::TiledLightingConfig def;
    return m_lightManager ? m_lightManager->GetTiledLightingConfig() : def;
}

const SecretEngine::LightCullingStats& LightingPlugin::GetCullingStats() const {
    static SecretEngine::LightCullingStats def;
    return m_lightManager ? m_lightManager->GetCullingStats() : def;
}

void LightingPlugin::SetTiledRenderingEnabled(bool e) {
    if (m_lightManager) m_lightManager->SetTiledRenderingEnabled(e);
}

bool LightingPlugin::IsTiledRenderingEnabled() const {
    return m_lightManager ? m_lightManager->IsTiledRenderingEnabled() : false;
}

extern "C" {
    SecretEngine::IPlugin* CreatePlugin()         { return new LightingPlugin(); }
    SecretEngine::IPlugin* CreateLightingPlugin() { return new LightingPlugin(); }
    void DestroyPlugin(SecretEngine::IPlugin* p)  { delete p; }
}
