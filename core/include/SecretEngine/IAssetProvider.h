// ============================================================================
// SECRET ENGINE - ASSET PROVIDER INTERFACE
// Abstracted binary and text loading (Android Assets / Windows Files)
// ============================================================================
#pragma once
#include <vector>
#include <string>

namespace SecretEngine {

class IAssetProvider {
public:
    virtual ~IAssetProvider() = default;

    // Load entire file into a binary buffer
    virtual std::vector<char> LoadBinary(const char* path) = 0;

    // Load file directly into a pre-allocated buffer (Zero-Copy)
    virtual bool LoadBinaryToBuffer(const char* path, void* dest, size_t size) = 0;

    // Load file as a string (convenience for shaders/configs)
    virtual std::string LoadText(const char* path) = 0;

    // Check if asset exists
    virtual bool Exists(const char* path) = 0;
};

} // namespace SecretEngine
