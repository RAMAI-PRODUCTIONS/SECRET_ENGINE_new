// SecretEngine
// Module: core
// Responsibility: Platform-agnostic asset loading implementation
// Dependencies: IAssetProvider, ILogger

#include <SecretEngine/IAssetProvider.h>
#include <SecretEngine/ILogger.h>
#include <fstream>
#include <vector>

#if defined(SE_PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
extern struct android_app* g_AndroidApp;
#endif

namespace SecretEngine {

class AssetProvider : public IAssetProvider {
public:
    AssetProvider(ILogger* logger) : m_logger(logger) {}

    std::vector<char> LoadBinary(const char* path) override {
        std::vector<char> buffer;
#if defined(SE_PLATFORM_ANDROID)
        if (g_AndroidApp && g_AndroidApp->activity && g_AndroidApp->activity->assetManager) {
            AAsset* asset = AAssetManager_open(g_AndroidApp->activity->assetManager, path, AASSET_MODE_BUFFER);
            if (asset) {
                const void* assetData = AAsset_getBuffer(asset);
                size_t size = AAsset_getLength(asset);
                if (assetData) {
                    // Stage 1 NITRO: Zero-Copy Direct Access
                    const char* dataPtr = static_cast<const char*>(assetData);
                    buffer.assign(dataPtr, dataPtr + size);
                } else {
                    // Fallback to read if buffer access fails
                    buffer.resize(size);
                    AAsset_read(asset, buffer.data(), size);
                }
                AAsset_close(asset);
                return buffer;
            }
        }
#else
        // Windows Path Resolution (Root, Assets/, ../Assets/)
        const char* fallbacks[] = { "", "Assets/", "../Assets/", "../../Assets/" };
        for (const char* prefix : fallbacks) {
            std::string fullPath = std::string(prefix) + path;
            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                size_t size = (size_t)file.tellg();
                buffer.resize(size);
                file.seekg(0);
                file.read(buffer.data(), size);
                file.close();
                return buffer;
            }
        }
#endif
        if (m_logger) m_logger->LogError("AssetProvider", (std::string("Failed to load asset: ") + path).c_str());
        return {};
    }

    bool LoadBinaryToBuffer(const char* path, void* dest, size_t size) override {
#if defined(SE_PLATFORM_ANDROID)
        if (g_AndroidApp && g_AndroidApp->activity && g_AndroidApp->activity->assetManager) {
            AAsset* asset = AAssetManager_open(g_AndroidApp->activity->assetManager, path, AASSET_MODE_BUFFER);
            if (asset) {
                const void* assetData = AAsset_getBuffer(asset);
                size_t assetSize = AAsset_getLength(asset);
                size_t toRead = (size < assetSize) ? size : assetSize;
                
                if (assetData) {
                    // NITRO: Direct memory-to-memory copy without intermediate buffering
                    memcpy(dest, assetData, toRead);
                } else {
                    AAsset_read(asset, dest, toRead);
                }
                AAsset_close(asset);
                return true;
            }
        }
#else
        const char* fallbacks[] = { "", "Assets/", "../Assets/", "../../Assets/" };
        for (const char* prefix : fallbacks) {
            std::string fullPath = std::string(prefix) + path;
            std::ifstream file(fullPath, std::ios::binary);
            if (file.is_open()) {
                file.read(reinterpret_cast<char*>(dest), size);
                file.close();
                return true;
            }
        }
#endif
        return false;
    }

    std::string LoadText(const char* path) override {
        auto binary = LoadBinary(path);
        return std::string(binary.begin(), binary.end());
    }

    bool Exists(const char* path) override {
#if defined(SE_PLATFORM_ANDROID)
        if (g_AndroidApp && g_AndroidApp->activity && g_AndroidApp->activity->assetManager) {
            AAsset* asset = AAssetManager_open(g_AndroidApp->activity->assetManager, path, AASSET_MODE_UNKNOWN);
            if (asset) {
                AAsset_close(asset);
                return true;
            }
        }
        return false;
#else
        const char* fallbacks[] = { "", "Assets/", "../Assets/", "../../Assets/" };
        for (const char* prefix : fallbacks) {
            std::string fullPath = std::string(prefix) + path;
            std::ifstream file(fullPath);
            if (file.is_open()) return true;
        }
        return false;
#endif
    }

private:
    ILogger* m_logger;
};

IAssetProvider* CreateAssetProvider(ILogger* logger) {
    return new AssetProvider(logger);
}

} // namespace SecretEngine
