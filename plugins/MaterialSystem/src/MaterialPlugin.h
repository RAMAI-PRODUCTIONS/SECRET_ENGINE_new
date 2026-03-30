#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/IMaterialSystem.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace SecretEngine {
class ICore;
}

// GPU-Driven Material System (Bindless, Zero CPU Overhead)
class MaterialPlugin : public SecretEngine::IPlugin, public SecretEngine::IMaterialSystem {
public:
    MaterialPlugin() = default;
    ~MaterialPlugin() override;
    
    // IPlugin
    const char* GetName() const override { return "MaterialSystem"; }
    uint32_t GetVersion() const override { return 1; }
    void OnLoad(SecretEngine::ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float deltaTime) override;
    
    // IMaterialSystem
    SecretEngine::MaterialHandle CreateMaterial(const char* name, const SecretEngine::MaterialProperties& props) override;
    void UpdateMaterial(SecretEngine::MaterialHandle handle, const SecretEngine::MaterialProperties& props) override;
    void DestroyMaterial(SecretEngine::MaterialHandle handle) override;
    const SecretEngine::MaterialProperties* GetMaterial(SecretEngine::MaterialHandle handle) const override;
    SecretEngine::MaterialHandle GetMaterialByName(const char* name) const override;
    std::span<const SecretEngine::MaterialProperties> GetMaterialBuffer() const override;
    const void* GetMaterialBufferRaw() const override;
    size_t GetMaterialBufferSize() const override;
    uint32_t GetMaterialCount() const override;
    
private:
    static constexpr uint32_t MAX_MATERIALS = 4096;
    
    SecretEngine::ICore* m_core = nullptr;
    std::vector<SecretEngine::MaterialProperties> m_materials;
    std::unordered_map<std::string, uint32_t> m_nameToIndex;
    std::vector<uint32_t> m_freeSlots;
    uint32_t m_nextGeneration = 1;
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
