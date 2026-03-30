// SecretEngine
// Module: core
// Responsibility: Plugin lifecycle interface
// Dependencies: <cstdint>

#pragma once
#include <cstdint>

namespace SecretEngine {
    class ICore;

    class IPlugin {
    public:
        virtual ~IPlugin() = default;
        virtual const char* GetName() const = 0;
        virtual uint32_t GetVersion() const = 0;
        virtual void OnLoad(ICore* core) = 0;
        virtual void OnActivate() = 0;
        virtual void OnDeactivate() = 0;
        virtual void OnUnload() = 0;
        virtual void OnUpdate(float dt) {}
        
        // RTTI-free interface access
        virtual void* GetInterface(uint32_t id) { return nullptr; }
    };
}