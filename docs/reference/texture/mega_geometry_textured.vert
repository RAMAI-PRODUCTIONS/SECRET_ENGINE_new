// ============================================================================
// MEGA GEOMETRY VERTEX SHADER - WITH TEXTURE SUPPORT
// Optimized instanced rendering with per-instance textures
// ============================================================================

#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_nonuniform_qualifier : enable

// ============================================================================
// INPUT (Per-vertex attributes - NITRO BALANCED 16-BIT)
// ============================================================================
layout(location = 0) in vec4 inPosition; // R16G16B16A16_SNORM
layout(location = 1) in vec4 inNormal;   // R8G8B8A8_SNORM
layout(location = 2) in vec2 inTexCoord; // R16G16_UNORM

// ============================================================================
// OUTPUT (To fragment shader)
// ============================================================================
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out flat uint fragTextureID;

// ============================================================================
// INSTANCE DATA (SSBO - Extended with texture ID)
// ============================================================================
struct InstanceData {
    vec4 row0; // Rotation + Trans X
    vec4 row1; // Rotation + Trans Y
    vec4 row2; // Rotation + Trans Z
    uint packedColor;      // RGBA8 packed color
    uint textureID;        // Bindless texture index
    uint _padding0;
    uint _padding1;
};

layout(std430, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// ============================================================================
// CAMERA UNIFORM (Push Constants)
// ============================================================================
layout(push_constant) uniform CameraData {
    mat4 viewProj;
} camera;

// ============================================================================
// LIGHTING CONSTANTS
// ============================================================================
const vec3 LIGHT_DIR = vec3(0.57735027, 0.57735027, 0.57735027);
const float AMBIENT = 0.3;
const float DIFFUSE_SCALE = 0.7;

void main() {
    // Fetch instance data
    InstanceData instance = instances[gl_InstanceIndex];
    
    // Decode vertex position
    vec3 localPos = inPosition.xyz * 8.0;
    
    // Transform to world space
    vec3 worldPos;
    worldPos.x = fma(instance.row0.x, localPos.x, fma(instance.row0.y, localPos.y, fma(instance.row0.z, localPos.z, instance.row0.w)));
    worldPos.y = fma(instance.row1.x, localPos.x, fma(instance.row1.y, localPos.y, fma(instance.row1.z, localPos.z, instance.row1.w)));
    worldPos.z = fma(instance.row2.x, localPos.x, fma(instance.row2.y, localPos.y, fma(instance.row2.z, localPos.z, instance.row2.w)));
    
    gl_Position = camera.viewProj * vec4(worldPos, 1.0);
    
    // Transform normal
    vec3 N;
    N.x = dot(vec3(instance.row0.x, instance.row0.y, instance.row0.z), inNormal.xyz);
    N.y = dot(vec3(instance.row1.x, instance.row1.y, instance.row1.z), inNormal.xyz);
    N.z = dot(vec3(instance.row2.x, instance.row2.y, instance.row2.z), inNormal.xyz);
    
    float invLen = inversesqrt(dot(N, N));
    N *= invLen;
    
    // Calculate lighting
    float lighting = fma(max(dot(N, LIGHT_DIR), 0.0), DIFFUSE_SCALE, AMBIENT);
    
    // Unpack color
    vec4 instanceColor = unpackUnorm4x8(instance.packedColor);
    
    // Output to fragment shader
    fragTexCoord = inTexCoord;
    fragNormal = N;
    fragColor = vec4(instanceColor.rgb * lighting, instanceColor.a);
    fragTextureID = instance.textureID;
}
