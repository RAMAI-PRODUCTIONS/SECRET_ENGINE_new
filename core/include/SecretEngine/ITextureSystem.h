#pragma once
#include <cstdint>
#include <cstddef>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {

struct TextureHandle {
    uint32_t id;
    uint32_t generation;
    
    bool IsValid() const { return id != 0; }
    static TextureHandle Invalid() { return {0, 0}; }
};

enum class TextureFormat {
    RGBA8_UNORM,
    RGBA8_SRGB,
    BC7_UNORM,
    BC7_SRGB,
    ASTC_4x4_UNORM,
    ASTC_4x4_SRGB,
    ASTC_8x8_UNORM,
    ASTC_8x8_SRGB
};

struct TextureDesc {
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    TextureFormat format;
    bool generateMips;
    bool streaming;
};

class ITextureSystem {
public:
    virtual ~ITextureSystem() = default;
    
    // Synchronous loading
    virtual TextureHandle LoadTexture(const char* path) = 0;
    
    // C++26: Type-safe texture creation
    virtual TextureHandle CreateTexture(const TextureDesc& desc, std::span<const std::byte> data) = 0;
    
    // Legacy API
    virtual TextureHandle CreateTextureRaw(const TextureDesc& desc, const void* data) = 0;
    
    // Asynchronous loading
    virtual TextureHandle LoadTextureAsync(const char* path) = 0;
    virtual bool IsTextureReady(TextureHandle handle) const = 0;
    
    // Management
    virtual void UnloadTexture(TextureHandle handle) = 0;
    virtual void* GetNativeHandle(TextureHandle handle) const = 0;
    
    // Streaming
    virtual void SetStreamingDistance(float distance) = 0;
    virtual void UpdateStreaming(std::span<const float, 3> cameraPos) = 0;
    
    // Stats
    virtual uint32_t GetLoadedTextureCount() const = 0;
    virtual size_t GetTextureMemoryUsage() const = 0;
};

} // namespace SecretEngine
