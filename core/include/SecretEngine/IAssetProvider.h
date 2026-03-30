// ============================================================================
// SECRET ENGINE - ASSET PROVIDER INTERFACE
// Abstracted binary and text loading (Android Assets / Windows Files)
// ============================================================================
#pragma once
#include <vector>
#include <string>
#include <SecretEngine/CPP26Features.h>

namespace SecretEngine {

class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;

    // Load entire file into a binary buffer
    virtual std::vector<char> LoadBinary(const char* path) = 0;

    // C++26: Load file directly into a span (type-safe, bounds-checked)
    virtual bool LoadBinaryToBuffer(const char* path, std::span<std::byte> dest) = 0;
    
    // Legacy API (deprecated, use span version)
    virtual bool LoadBinaryToBufferRaw(const char* path, void* dest, size_t size) = 0;

    // Load file as a string (convenience for shaders/configs)
    virtual std::string LoadText(const char* path) = 0;

    // Check if asset exists
    virtual bool Exists(const char* path) = 0;
};

} // namespace SecretEngine
