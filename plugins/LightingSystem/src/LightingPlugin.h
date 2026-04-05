#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ILightingSystem.h>
#include <SecretEngine/Fast/FastData.h>
#include <unordered_map>

namespace SecretEngine { class ICore; class LightManager; }

class LightingPlugin : public SecretEngine::IPlugin, public SecretEngine::ILightingSystem {
public:
    LightingPlugin() = default;
    ~LightingPlugin() override;
    
    const char* GetName() const override { return "LightingSystem"; }
    uint32_t GetVersion() const override { return 1; }
    void OnLoad(SecretEngine::ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    
    void* GetInterface(uint32_t id) override {
        if (id == 3) return static_cast<SecretEngine::ILightingSystem*>(this);
        return nullptr;
    }
    
    // ILightingSystem
    uint32_t AddLight(const SecretEngine::LightData& light) override;
    void UpdateLight(uint32_t lightID, const SecretEngine::LightData& light) override;
    void RemoveLight(uint32_t lightID) override;
    const SecretEngine::LightData* GetLight(uint32_t lightID) const override;
    uint32_t GetLightCount() const override;
    std::span<const SecretEngine::LightData> GetLightBuffer() const override;
    const void* GetLightBufferRaw() const override;
    size_t GetLightBufferSize() const override;
    void SetTiledLightingConfig(const SecretEngine::TiledLightingConfig& config) override;
    const SecretEngine::TiledLightingConfig& GetTiledLightingConfig() const override;
    const SecretEngine::LightCullingStats& GetCullingStats() const override;
    void SetTiledRenderingEnabled(bool enabled) override;
    bool IsTiledRenderingEnabled() const override;

    // FDA stream pointer set by RendererPlugin after Vulkan init
    void SetFDAStream(SecretEngine::Fast::UltraRingBuffer<2048>* stream) { m_fdaStream = stream; }
    void* GetLightUpdateStream() override { return m_fdaStream; }

    // lightID → slot index in MegaLightRenderer SSBO
    uint32_t GetSlotForLight(uint32_t lightID) const {
        auto it = m_idToSlot.find(lightID);
        return it != m_idToSlot.end() ? it->second : UINT32_MAX;
    }
    uint32_t AllocSlot(uint32_t lightID) {
        uint32_t slot = m_nextSlot++;
        m_idToSlot[lightID] = slot;
        return slot;
    }

private:
    SecretEngine::ICore* m_core = nullptr;
    SecretEngine::LightManager* m_lightManager = nullptr;
    SecretEngine::Fast::UltraRingBuffer<2048>* m_fdaStream = nullptr;
    std::unordered_map<uint32_t, uint32_t> m_idToSlot;
    uint32_t m_nextID   = 1;
    uint32_t m_nextSlot = 0;
};

extern "C" {
#ifdef _WIN32
    __declspec(dllexport) SecretEngine::IPlugin* CreatePlugin();
    __declspec(dllexport) void DestroyPlugin(SecretEngine::IPlugin* plugin);
#else
    SecretEngine::IPlugin* CreatePlugin();
    void DestroyPlugin(SecretEngine::IPlugin* plugin);
#endif
}
