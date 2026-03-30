// ============================================================================
// MEGA GEOMETRY FRAGMENT SHADER
// Simple PBR lighting for millions of triangles
// ============================================================================

#version 450

// ============================================================================
// INPUT (From vertex shader)
// ============================================================================
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;

// ============================================================================
// OUTPUT
// ============================================================================
layout(location = 0) out vec4 outColor;

// ============================================================================
// LIGHTING (Simple directional light)
// ============================================================================
const vec3 LIGHT_DIR = normalize(vec3(1.0, 1.0, 1.0));
const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const float AMBIENT = 0.3;

void main() {
    // Normalize interpolated normal
    vec3 N = normalize(fragNormal);
    
    // Diffuse lighting
    float diff = max(dot(N, LIGHT_DIR), 0.0);
    vec3 lighting = AMBIENT + (1.0 - AMBIENT) * diff * LIGHT_COLOR;
    
    // Apply lighting to instance color
    vec3 color = fragColor.rgb * lighting;
    
    outColor = vec4(color, fragColor.a);
}
