// SecretEngine
// Module: core
// Responsibility: Implementation of plugin scanning and dynamic loading
// Dependencies: PluginManager.h, <fstream>, nlohmann/json

#include "PluginManager.h"
#include <SecretEngine/ICore.h>
#include <nlohmann/json.hpp>
#include <fstream>

#if defined(SE_PLATFORM_WINDOWS)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace SecretEngine {

    void PluginManager::ScanPlugins(const std::filesystem::path& path, ICore* core) {
        if (!std::filesystem::exists(path)) return;

        for (auto const& dir_entry : std::filesystem::directory_iterator{path}) {
            if (dir_entry.is_directory()) {
                std::filesystem::path manifestPath = dir_entry.path() / "plugin_manifest.json";
                if (std::filesystem::exists(manifestPath)) {
                    // Parse manifest
                    std::ifstream f(manifestPath);
                    nlohmann::json manifest = nlohmann::json::parse(f);
                    std::string libName = manifest["library"];
                    std::filesystem::path libPath = dir_entry.path() / libName;

                    // Platform-specific LoadLibrary
                    void* handle = nullptr;
#if defined(SE_PLATFORM_WINDOWS)
                    handle = LoadLibraryW(libPath.c_str());
#else
                    handle = dlopen(libPath.c_str(), RTLD_NOW);
#endif

                    if (handle) {
                        // Resolve CreatePlugin symbol
                        typedef IPlugin* (*CreateFunc)();
#if defined(SE_PLATFORM_WINDOWS)
                        CreateFunc create = (CreateFunc)GetProcAddress((HMODULE)handle, "CreatePlugin");
#else
                        CreateFunc create = (CreateFunc)dlsym(handle, "CreatePlugin");
#endif

                        if (create) {
                            IPlugin* plugin = create();
                            plugin->OnLoad(core); // Phase 3: Register
                            m_loadedPlugins.push_back({ handle, plugin });
                        }
                    }
                }
            }
        }
    }

    void PluginManager::UnloadAll() {
        for (auto& lp : m_loadedPlugins) {
            lp.pluginInstance->OnUnload(); // Phase 7: Unload
            // In a full implementation, we would call DestroyPlugin here
#if defined(SE_PLATFORM_WINDOWS)
            FreeLibrary((HMODULE)lp.libraryHandle);
#else
            dlclose(lp.libraryHandle);
#endif
        }
        m_loadedPlugins.clear();
    }

    PluginManager::~PluginManager() { UnloadAll(); }
}