#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/Fast/FastData.h>

namespace SecretEngine {
    class IRenderer : public IPlugin { 
    public:
        virtual ~IRenderer() = default;
        
        // Fast Command Protocol
        virtual Fast::UltraRingBuffer<1024>& GetCommandStream() = 0;
        
        virtual void Submit() = 0;
        virtual void Present() = 0;
        virtual void InitializeHardware(void* nativeWindow) = 0;
        virtual void SetCubeColor(int colorIndex) = 0;
        virtual void SetDebugInfo(int slot, const char* text) {}
        virtual void GetStats(uint32_t& instances, uint32_t& triangles, uint32_t& drawCalls) {}
        virtual uint64_t GetVRAMUsage() { return 0; }
        virtual void ClearAllInstances() {}

        // Particle / instance API (used by game logic for light particles etc.)
        virtual uint32_t SpawnInstance(const char* meshPath, float x, float y, float z,
                                       float r, float g, float b, float scale = 1.0f) { return UINT32_MAX; }
        virtual void UpdateInstancePosColor(uint32_t id, float x, float y, float z,
                                            float r, float g, float b, float scale = 1.0f) {}
        virtual void DespawnInstance(uint32_t id, const char* meshPath) {}
    };
}
