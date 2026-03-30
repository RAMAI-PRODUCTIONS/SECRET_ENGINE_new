// SecretEngine
// Module: tests
// Responsibility: Basic engine initialization and hardware activation test

#include <SecretEngine/Core.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ILogger.h>

int main() {
    SecretEngine::ICore* core = SecretEngine::GetEngineCore();
    
    if (core) {
        // 1. Initialize Core (Scans and Loads Plugins)
        core->Initialize();
        
        SecretEngine::ILogger* logger = core->GetLogger();
        
        // 2. Locate the Rendering Capability
        auto renderer = core->GetCapability("rendering");
        if (renderer) {
            logger->LogInfo("Test", "Found 'rendering' capability. Activating hardware...");
            renderer->OnActivate(); // This opens the window
        } else {
            logger->LogError("Test", "Failed to find 'rendering' capability! Check DLL paths.");
        }
        
        // 3. Main Loop
        logger->LogInfo("Test", "Entering main loop...");
        while (!core->ShouldClose()) {
            core->Update(); // Handles PollEvents via Submit()
        }
        
        // 4. Clean Shutdown
        core->Shutdown();
    }
    
    return 0;
}