// SecretEngine
// Module: core
// Responsibility: Common POD components for the ECS
#pragma once

namespace SecretEngine {
    struct TransformComponent {
        float position[3] = {0,0,0};
        float rotation[3] = {0,0,0};
        float scale[3] = {1,1,1};
        
        static const uint32_t TypeID = 0x0001;
    };

    struct MeshComponent {
        char meshPath[256] = {0};
        float color[4] = {1,1,1,1}; // RGBA
        // Optional texture paths (relative to Assets/, e.g. "textures/diffuse.jpeg")
        char texturePath[256] = {0};
        char normalMapPath[256] = {0};
        
        static const uint32_t TypeID = 0x0002;
    };
}
