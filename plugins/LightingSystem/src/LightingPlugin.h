#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ILightingSystem.h>

namespace SecretEngine {
class ICore;
class LightManager;
}

class LightingPlugin : public SecretEngine::IPlugin, public SecretEngine::ILightingSystem {
public:
    LightingPlugin() = default;
    ~LightingPlugin() override;
    
    // IPlugin interface
    const char* GetName() const override { return "LightingSystem"; }
    uint32_t GetVersion() const override { return 1; }
    void OnLoad(SecretEngine::ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float deltaTime) override;
    
    // ILightingSystem interface
    uint32_t AddLight(const SecretEngine::LightData& light) override;
    void UpdateLight(uint32_t lightID, const SecretEngine::LightData& light) override;
    void RemoveLight(uint32_t lightID) override;
    const SecretEngine::LightData* GetLight(uint32_t lightID) const override;
    uint32_t GetLightCount() const override;
    std::span<const SecretEngine::LightData> GetLightBuffer() const override;
    const void* GetLightBufferRaw() const override;
    size_t GetLightBufferSize() const override;
    
private:
    SecretEngine::ICore* m_core = nullptr;
    SecretEngine::LightManager* m_lightManager = nullptr;
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
