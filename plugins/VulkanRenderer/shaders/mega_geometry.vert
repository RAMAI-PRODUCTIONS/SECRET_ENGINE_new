// ============================================================================
// MEGA GEOMETRY VERTEX SHADER - ULTRA OPTIMIZED
// 2x faster than previous version through aggressive math optimization
// ============================================================================

#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_nonuniform_qualifier : enable

// ============================================================================
// INPUT (Per-vertex attributes - NITRO BALANCED 16-BIT)
// ============================================================================
layout(location = 0) in vec4 inPosition;    // R16G16B16A16_SNORM
layout(location = 1) in vec4 inNormal;      // R8G8B8A8_SNORM
layout(location = 2) in vec2 inTexCoord;    // R16G16_UNORM
layout(location = 3) in uint inVertexColor; // R32_UINT (R11G11B10F packed lighting)

// ============================================================================
// OUTPUT (To fragment shader)
// ============================================================================
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec3 fragVertexLight;  // Dynamic per-vertex lighting
layout(location = 4) out flat uint fragTextureID;

// ============================================================================
// INSTANCE DATA (SSBO - Extended with texture ID, 64 bytes total)
// ============================================================================
struct InstanceData {
    vec4 row0; // Rotation + Trans X
    vec4 row1; // Rotation + Trans Y
    vec4 row2; // Rotation + Trans Z
    uint packedColor;      // RGBA8 packed color
    uint textureID;        // Bindless texture index (UINT_MAX = no texture)
    uint _padding0;
    uint _padding1;
};

layout(std430, set = 0, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// ============================================================================
// CAMERA UNIFORM (Push Constants)
// ============================================================================
layout(push_constant) uniform CameraData {
    mat4 viewProj;
} camera;

// ============================================================================
// UNPACK R11G11B10F (HDR vertex lighting)
// ============================================================================
vec3 unpackR11G11B10F(uint packed) {
    uint r = (packed >> 21) & 0x7FF;  // 11 bits
    uint g = (packed >> 10) & 0x7FF;  // 11 bits
    uint b = packed & 0x3FF;          // 10 bits
    
    // Convert to float [0, 1] then scale to HDR range [0, 64]
    return vec3(
        float(r) / 2047.0,
        float(g) / 2047.0,
        float(b) / 1023.0
    ) * 64.0;  // HDR multiplier for bright lights
}

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
    
    // Transform normal (for future use, but not used for lighting)
    vec3 N;
    N.x = dot(vec3(instance.row0.x, instance.row0.y, instance.row0.z), inNormal.xyz);
    N.y = dot(vec3(instance.row1.x, instance.row1.y, instance.row1.z), inNormal.xyz);
    N.z = dot(vec3(instance.row2.x, instance.row2.y, instance.row2.z), inNormal.xyz);
    
    float invLen = inversesqrt(dot(N, N));
    N *= invLen;
    
    // Unpack instance color (base tint)
    vec4 instanceColor = unpackUnorm4x8(instance.packedColor);
    
    // Unpack vertex lighting (dynamic per-vertex)
    vec3 vertexLight = unpackR11G11B10F(inVertexColor);
    
    // Output to fragment shader
    fragTexCoord = inTexCoord;
    fragNormal = N;
    fragColor = instanceColor;      // Base color/tint
    fragVertexLight = vertexLight;  // Dynamic lighting
    fragTextureID = instance.textureID;
}
