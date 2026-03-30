// SecretEngine/ICore.h
#pragma once

namespace SecretEngine {
    class IAllocator;
    class ILogger;
    class IPlugin;

    class ICore {
    public:
        virtual ~ICore() = default;
        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        
        // --- Added for Day 2 Game Loop ---
        virtual void Update() = 0;           // Processes one engine frame
        virtual bool ShouldClose() const = 0; // Signals if the engine should exit
        virtual void SetRendererReady(bool ready) = 0; // Signals when the renderer is ready to present
        // ---------------------------------

        virtual IAllocator* GetAllocator(const char* name) = 0;
        virtual ILogger* GetLogger() = 0;
        virtual class IWorld* GetWorld() = 0;
        virtual class IInputSystem* GetInput() = 0;
        virtual class IAssetProvider* GetAssetProvider() = 0;
        virtual void RegisterCapability(const char* name, IPlugin* plugin) = 0;
        virtual IPlugin* GetCapability(const char* name) = 0;
    };
}