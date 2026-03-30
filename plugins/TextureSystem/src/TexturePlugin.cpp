#include "TexturePlugin.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <cstring>

TexturePlugin::~TexturePlugin() {
    m_running = false;
    if (m_loaderThread.joinable()) {
        m_loaderThread.join();
    }
}

void TexturePlugin::OnLoad(SecretEngine::ICore* core) {
    m_core = core;
    m_textures.reserve(MAX_TEXTURES);
    
    // Start async loader thread
    m_running = true;
    m_loaderThread = std::thread(&TexturePlugin::LoaderThreadFunc, this);
    
    m_core->RegisterCapability("textures", this);
    
    if (m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("TextureSystem", "✓ Async Texture System initialized (2048 textures, streaming enabled)");
    }
}

void TexturePlugin::OnActivate() {
}

void TexturePlugin::OnDeactivate() {
}

void TexturePlugin::OnUnload() {
    m_running = false;
    if (m_loaderThread.joinable()) {
        m_loaderThread.join();
    }
}

void TexturePlugin::OnUpdate(float deltaTime) {
    // Update streaming based on camera position
    // TODO: Implement distance-based texture streaming
}

SecretEngine::TextureHandle TexturePlugin::LoadTexture(const char* path) {
    // Check cache
    auto it = m_pathToIndex.find(path);
    if (it != m_pathToIndex.end()) {
        return {it->second, 0};
    }
    
    // Allocate slot
    uint32_t index;
    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
    } else {
        if (m_textures.size() >= MAX_TEXTURES) {
            return SecretEngine::TextureHandle::Invalid();
        }
        index = static_cast<uint32_t>(m_textures.size());
        m_textures.push_back(TextureData());
    }
    
    // TODO: Actual texture loading (integrate with renderer backend)
    m_textures[index].isReady = true;
    m_pathToIndex[path] = index;
    
    return {index, m_nextGeneration++};
}

SecretEngine::TextureHandle TexturePlugin::CreateTexture(const SecretEngine::TextureDesc& desc, const void* data) {
    uint32_t index;
    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
    } else {
        if (m_textures.size() >= MAX_TEXTURES) {
            return SecretEngine::TextureHandle::Invalid();
        }
        index = static_cast<uint32_t>(m_textures.size());
        m_textures.push_back(TextureData());
    }
    
    m_textures[index].width = desc.width;
    m_textures[index].height = desc.height;
    m_textures[index].mipLevels = desc.mipLevels;
    m_textures[index].format = desc.format;
    m_textures[index].isReady = true;
    
    return {index, m_nextGeneration++};
}

SecretEngine::TextureHandle TexturePlugin::LoadTextureAsync(const char* path) {
    // Check cache
    auto it = m_pathToIndex.find(path);
    if (it != m_pathToIndex.end()) {
        return {it->second, 0};
    }
    
    // Allocate slot
    uint32_t index;
    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
    } else {
        if (m_textures.size() >= MAX_TEXTURES) {
            return SecretEngine::TextureHandle::Invalid();
        }
        index = static_cast<uint32_t>(m_textures.size());
        m_textures.push_back(TextureData());
    }
    
    m_textures[index].isReady = false;
    m_pathToIndex[path] = index;
    
    // Queue for async loading
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_loadQueue.push({path, index});
    }
    
    return {index, m_nextGeneration++};
}

bool TexturePlugin::IsTextureReady(SecretEngine::TextureHandle handle) const {
    return (handle.id < m_textures.size()) ? m_textures[handle.id].isReady : false;
}

void TexturePlugin::UnloadTexture(SecretEngine::TextureHandle handle) {
    if (handle.id < m_textures.size()) {
        m_freeSlots.push_back(handle.id);
        m_textures[handle.id] = TextureData();
    }
}

void* TexturePlugin::GetNativeHandle(SecretEngine::TextureHandle handle) const {
    return (handle.id < m_textures.size()) ? m_textures[handle.id].nativeHandle : nullptr;
}

void TexturePlugin::SetStreamingDistance(float distance) {
    m_streamingDistance = distance;
}

void TexturePlugin::UpdateStreaming(const float cameraPos[3]) {
    memcpy(m_cameraPos, cameraPos, sizeof(float) * 3);
}

uint32_t TexturePlugin::GetLoadedTextureCount() const {
    uint32_t count = 0;
    for (const auto& tex : m_textures) {
        if (tex.isReady) count++;
    }
    return count;
}

size_t TexturePlugin::GetTextureMemoryUsage() const {
    size_t total = 0;
    for (const auto& tex : m_textures) {
        total += tex.memorySize;
    }
    return total;
}

void TexturePlugin::LoaderThreadFunc() {
    while (m_running) {
        LoadRequest request;
        bool hasRequest = false;
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_loadQueue.empty()) {
                request = m_loadQueue.front();
                m_loadQueue.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest) {
            ProcessLoadRequest(request);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void TexturePlugin::ProcessLoadRequest(const LoadRequest& request) {
    // TODO: Actual async texture loading
    // For now, just mark as ready
    if (request.textureID < m_textures.size()) {
        m_textures[request.textureID].isReady = true;
    }
}

extern "C" {
    SecretEngine::IPlugin* CreatePlugin() {
        return new TexturePlugin();
    }
    
    void DestroyPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
