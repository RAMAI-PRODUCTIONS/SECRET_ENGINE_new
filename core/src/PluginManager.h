// SecretEngine
// Module: core
// Responsibility: Plugin discovery and lifecycle management
// Dependencies: SecretEngine/IPlugin.h, <filesystem>, nlohmann/json

#pragma once
#include <SecretEngine/IPlugin.h> // Fix: Ensure this path is resolved
#include <vector>
#include <filesystem>

namespace SecretEngine {

    class PluginManager {
    public:
        PluginManager() = default;
        ~PluginManager();

        // Phase 1 & 2: Discovery and Library Loading
        void ScanPlugins(const std::filesystem::path& path, class ICore* core);
        
        // Phase 7: Unload all libraries
        void UnloadAll();

    private:
        struct LoadedPlugin {
            void* libraryHandle;
            IPlugin* pluginInstance;
        };
        std::vector<LoadedPlugin> m_loadedPlugins;
    };

}