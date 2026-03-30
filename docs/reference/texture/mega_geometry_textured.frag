// ============================================================================
// MEGA GEOMETRY FRAGMENT SHADER - WITH BINDLESS TEXTURES
// High-performance texture sampling with per-instance texture selection
// ============================================================================

#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// ============================================================================
// INPUT (From vertex shader)
// ============================================================================
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in flat uint fragTextureID;

// ============================================================================
// BINDLESS TEXTURE ARRAY
// ============================================================================
layout(binding = 1) uniform sampler2D textures[16384];

// ============================================================================
// OUTPUT
// ============================================================================
layout(location = 0) out vec4 outColor;

void main() {
    // Sample texture using bindless index
    vec4 texColor = vec4(1.0);
    
    if (fragTextureID < 16384) {
        texColor = texture(textures[nonuniformEXT(fragTextureID)], fragTexCoord);
    }
    
    // Combine texture with vertex color and lighting
    outColor = fragColor * texColor;
    
    // Optional: Alpha test for transparency
    if (outColor.a < 0.01) {
        discard;
    }
}
