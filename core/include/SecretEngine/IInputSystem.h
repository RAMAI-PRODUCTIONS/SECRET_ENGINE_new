#pragma once
#include <SecretEngine/Fast/FastData.h>

namespace SecretEngine {
    class IInputSystem {
    public:
        virtual ~IInputSystem() = default;
        virtual void Update() = 0;
        
        // Fast Protocol Access
        virtual Fast::UltraRingBuffer<512>& GetFastStream() = 0;
        
        // Legacy Polling (Optional fallback)
        virtual bool IsKeyPressed(int key) = 0;
        virtual bool IsMouseButtonPressed(int button) = 0;
        virtual void GetMousePosition(float& x, float& y) = 0;
    };
}
