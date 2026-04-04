// SecretEngine
// Module: core
// Responsibility: Main engine implementation and service registry
// Dependencies: ICore, SystemAllocator, Logger, PluginManager, IRenderer

#include <SecretEngine/Core.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IRenderer.h> // FIX: Resolve C2061 identifier 'IRenderer'
#include <SecretEngine/IWorld.h>
#include <SecretEngine/IInputSystem.h>
#include <SecretEngine/IAssetProvider.h>
#include "SystemAllocator.h"
#include "Logger.h"
#include "PluginManager.h"
#include <map>
#include <string>

#if defined(SE_PLATFORM_ANDROID)
extern "C" SecretEngine::IPlugin* CreateVulkanRendererPlugin();
extern "C" SecretEngine::IPlugin* CreateInputPlugin();
extern "C" SecretEngine::IPlugin* CreateCameraPlugin();
extern "C" SecretEngine::IPlugin* CreateLogicPlugin();
extern "C" SecretEngine::IPlugin* CreateDebugPlugin();
extern "C" SecretEngine::IPlugin* CreatePhysicsPlugin();
extern "C" SecretEngine::IPlugin* CreateGameplayTagPlugin();
extern "C" SecretEngine::IPlugin* CreateLevelSystemPlugin();
extern "C" SecretEngine::IPlugin* CreateFPSGamePlugin();
extern "C" SecretEngine::IPlugin* CreateFPSUIPlugin();
extern "C" SecretEngine::IPlugin* CreateProceduralCityPlugin();
extern "C" SecretEngine::IPlugin* CreateParticleSystemPlugin();
#endif

#include <chrono>

namespace SecretEngine {
    extern IWorld* CreateWorld();
    extern IAssetProvider* CreateAssetProvider(ILogger* logger);

    class Core : public ICore {
    public:
        static Core* GetInstance() {
            static Core instance;
            return &instance;
        }

        void Initialize() override {
            m_allocator = new SystemAllocator();
            m_logger = new Logger();
            m_assetProvider = CreateAssetProvider(m_logger);
            m_pluginManager = new PluginManager();
            m_world = CreateWorld();
            m_lastTime = std::chrono::high_resolution_clock::now();
            m_logger->LogInfo("Core", "SecretEngine Initializing...");
            
#if defined(SE_PLATFORM_ANDROID)
            IPlugin* renderer = CreateVulkanRendererPlugin();
            if (renderer) {
                renderer->OnLoad(this);
            }
            
            IPlugin* input = CreateInputPlugin();
            if (input) {
                input->OnLoad(this);
            }
            
            IPlugin* camera = CreateCameraPlugin();
            if (camera) {
                camera->OnLoad(this);
            }
            
            IPlugin* logic = CreateLogicPlugin();
            if (logic) {
                logic->OnLoad(this);
            }

            IPlugin* debug = CreateDebugPlugin();
            if (debug) {
                debug->OnLoad(this);
            }

            IPlugin* physics = CreatePhysicsPlugin();
            if (physics) {
                physics->OnLoad(this);
            }

            IPlugin* gameplayTags = CreateGameplayTagPlugin();
            if (gameplayTags) {
                gameplayTags->OnLoad(this);
            }

            IPlugin* levelSystem = CreateLevelSystemPlugin();
            if (levelSystem) {
                levelSystem->OnLoad(this);
            }

            IPlugin* fpsGame = CreateFPSGamePlugin();
            if (fpsGame) {
                fpsGame->OnLoad(this);
            }

            IPlugin* fpsUI = CreateFPSUIPlugin();
            if (fpsUI) {
                fpsUI->OnLoad(this);
            }

            IPlugin* proceduralCity = CreateProceduralCityPlugin();
            if (proceduralCity) {
                proceduralCity->OnLoad(this);
            }

            IPlugin* particleSystem = CreateParticleSystemPlugin();
            if (particleSystem) {
                particleSystem->OnLoad(this);
            }

            // Activate all loaded plugins
            for (auto const& [name, plugin] : m_capabilities) {
                if (plugin) {
                    plugin->OnActivate();
                }
            }
#else
            m_pluginManager->ScanPlugins("plugins", this); 
#endif
        }

        void Update() override {
            // Calculate Delta Time
            auto currentTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(currentTime - m_lastTime).count();
            m_lastTime = currentTime;

            // Update all plugins (includes camera, renderer, logic)
            for (auto const& [name, plugin] : m_capabilities) {
                plugin->OnUpdate(dt);
            }

            auto plugin = GetCapability("rendering");
            if (plugin && m_isRendererReady) {
                IRenderer* renderer = static_cast<IRenderer*>(plugin->GetInterface(1));
                if (renderer) {
                    renderer->Submit(); 
                    renderer->Present();
                }
                
                static bool firstFrame = true;
                if (firstFrame) {
                    if (m_logger) m_logger->LogInfo("Core", "First frame rendered!");
                    firstFrame = false;
                }
            }
        }

        void SetRendererReady(bool ready) override {
            m_isRendererReady = ready;
        }

        bool ShouldClose() const override {
            return m_shouldClose;
        }

        void Shutdown() override {
            m_logger->LogInfo("Core", "SecretEngine Shutting down...");
            if (m_pluginManager) {
                m_pluginManager->UnloadAll();
                delete m_pluginManager;
            }
            if (m_world) delete m_world;
            if (m_assetProvider) delete m_assetProvider;
            delete m_logger;
            delete m_allocator;
        }

        IAllocator* GetAllocator(const char* name) override { return m_allocator; }
        ILogger* GetLogger() override { return m_logger; }
        IWorld* GetWorld() override { return m_world; }
        IInputSystem* GetInput() override { 
            auto input = GetCapability("input");
            return input ? static_cast<IInputSystem*>(input->GetInterface(2)) : nullptr;
        }

        IAssetProvider* GetAssetProvider() override {
            return m_assetProvider;
        }

        void RegisterCapability(const char* name, IPlugin* plugin) override {
            m_capabilities[name] = plugin;
            m_logger->LogInfo("Core", ("Registered Capability: " + std::string(name)).c_str());
        }

        IPlugin* GetCapability(const char* name) override {
            auto it = m_capabilities.find(name);
            return (it != m_capabilities.end()) ? it->second : nullptr;
        }

    private:
        Core() : m_allocator(nullptr), m_logger(nullptr), m_assetProvider(nullptr), m_pluginManager(nullptr), m_world(nullptr), m_shouldClose(false), m_isRendererReady(false) {}
        SystemAllocator* m_allocator;
        Logger* m_logger;
        IAssetProvider* m_assetProvider;
        PluginManager* m_pluginManager;
        IWorld* m_world;
        std::map<std::string, IPlugin*> m_capabilities;
        bool m_shouldClose; 
        bool m_isRendererReady;
        std::chrono::high_resolution_clock::time_point m_lastTime;
    };

    ICore* GetEngineCore() {
        return Core::GetInstance();
    }
}