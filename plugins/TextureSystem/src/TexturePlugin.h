#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ITextureSystem.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>
#include <queue>
#include <atomic>

namespace SecretEngine {
class ICore;
}

struct TextureData {
    void* nativeHandle = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 0;
    SecretEngine::TextureFormat format;
    size_t memorySize = 0;
    bool isReady = false;
    bool isStreaming = false;
    float streamingDistance = 0.0f;
};

struct LoadRequest {
    std::string path;
    uint32_t textureID;
};

// Async Texture Loading System (Background Thread Pool)
class TexturePlugin : public SecretEngine::IPlugin, public SecretEngine::ITextureSystem {
public:
    TexturePlugin() = default;
    ~TexturePlugin() override;
    
    // IPlugin
    const char* GetName() const override { return "TextureSystem"; }
    uint32_t GetVersion() const override { return 1; }
    void OnLoad(SecretEngine::ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float deltaTime) override;
    
    // ITextureSystem
    SecretEngine::TextureHandle LoadTexture(const char* path) override;
    SecretEngine::TextureHandle CreateTexture(const SecretEngine::TextureDesc& desc, const void* data) override;
    SecretEngine::TextureHandle LoadTextureAsync(const char* path) override;
    bool IsTextureReady(SecretEngine::TextureHandle handle) const override;
    void UnloadTexture(SecretEngine::TextureHandle handle) override;
    void* GetNativeHandle(SecretEngine::TextureHandle handle) const override;
    void SetStreamingDistance(float distance) override;
    void UpdateStreaming(const float cameraPos[3]) override;
    uint32_t GetLoadedTextureCount() const override;
    size_t GetTextureMemoryUsage() const override;
    
private:
    void LoaderThreadFunc();
    void ProcessLoadRequest(const LoadRequest& request);
    
    SecretEngine::ICore* m_core = nullptr;
    std::vector<TextureData> m_textures;
    std::unordered_map<std::string, uint32_t> m_pathToIndex;
    std::vector<uint32_t> m_freeSlots;
    uint32_t m_nextGeneration = 1;
    
    // Async loading
    std::thread m_loaderThread;
    std::queue<LoadRequest> m_loadQueue;
    std::mutex m_queueMutex;
    std::atomic<bool> m_running{false};
    
    // Streaming
    float m_streamingDistance = 1000.0f;
    float m_cameraPos[3] = {0, 0, 0};
    
    static constexpr uint32_t MAX_TEXTURES = 2048;
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
