// ============================================================================
// MEGA GEOMETRY VERTEX SHADER
// Reads instance transforms directly from SSBO for millions of instances
// ============================================================================

#version 450

// ============================================================================
// INPUT (Per-vertex attributes)
// ============================================================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// ============================================================================
// OUTPUT (To fragment shader)
// ============================================================================
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;

// ============================================================================
// INSTANCE DATA (SSBO - GPU-Resident)
// ============================================================================
struct InstanceData {
    mat4 transform;   // 64 bytes
    vec4 color;       // 16 bytes
    // Total: 80 bytes
};

layout(std140, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// ============================================================================
// CAMERA UNIFORM (Push Constants)
// ============================================================================
layout(push_constant) uniform CameraData {
    mat4 viewProj;
} camera;

// ============================================================================
// MAIN VERTEX SHADER
// ============================================================================
void main() {
    // Read instance data from SSBO using gl_InstanceIndex
    InstanceData instance = instances[gl_InstanceIndex];
    
    // Transform vertex position
    vec4 worldPos = instance.transform * vec4(inPosition, 1.0);
    gl_Position = camera.viewProj * worldPos;
    
    // Transform normal
    mat3 normalMatrix = mat3(transpose(inverse(instance.transform)));
    fragNormal = normalize(normalMatrix * inNormal);
    
    // Pass through texture coordinates
    fragTexCoord = inTexCoord;
    
    // Pass through instance color
    fragColor = instance.color;
}
