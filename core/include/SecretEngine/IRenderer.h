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
        virtual uint64_t GetVRAMUsage() { return 0; } // Returns VRAM usage in bytes
    };
}
