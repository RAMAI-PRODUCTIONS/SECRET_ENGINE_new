// ============================================================================
// VERTEX COLOR LIGHTING INTEGRATION
// Complete implementation for vertex-color-only lighting (no textures)
// ============================================================================

#include "SimpleVertexLighting.h"
#include "Pipeline3D.h"
#include <SecretEngine/ILogger.h>
#include <SecretEngine/ICore.h>
#include <cstring>

namespace SecretEngine {

using namespace SecretEngine::Math;

// ============================================================================
// GLOBAL SCENE INSTANCE (for easy access)
// ============================================================================
static SimpleVertexLighting g_lighting;

// ============================================================================
// PUBLIC API
// ============================================================================

void InitializeVertexColorLighting(ICore* core) {
    g_lighting.ClearLights();
    g_lighting.SetAmbient(Float3{0.2f, 0.2f, 0.3f}, 0.15f);
    g_lighting.AddDirectionalLight(Float3{0.3f, -0.8f, 0.5f}, Float3{1.0f, 0.95f, 0.8f}, 1.2f);
    
    if (core && core->GetLogger()) {
        core->GetLogger()->LogInfo("VertexLighting", 
            "✓ Vertex color lighting initialized (no textures, pure vertex colors)");
    }
}

void SetLightingPreset(const char* preset) {
    // Presets can be implemented here if needed
}

} // namespace SecretEngine
