// ============================================================================
// MEGA GEOMETRY VERTEX SHADER
// ============================================================================

#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 inPosition; // R16G16B16A16_SNORM
layout(location = 1) in vec4 inNormal;   // R8G8B8A8_SNORM
layout(location = 2) in vec2 inTexCoord; // R16G16_UNORM

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out flat uint fragTextureID;
layout(location = 4) out vec3 fragWorldPos;

struct InstanceData {
    vec4 row0;
    vec4 row1;
    vec4 row2;
    uint packedColor;
    uint textureID;
    uint _padding0;
    uint _padding1;
};

layout(std430, set = 0, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(push_constant) uniform CameraData {
    mat4 viewProj;
} camera;

void main() {
    InstanceData instance = instances[gl_InstanceIndex];

    vec3 localPos = inPosition.xyz * 8.0;

    vec3 worldPos;
    worldPos.x = fma(instance.row0.x, localPos.x, fma(instance.row0.y, localPos.y, fma(instance.row0.z, localPos.z, instance.row0.w)));
    worldPos.y = fma(instance.row1.x, localPos.x, fma(instance.row1.y, localPos.y, fma(instance.row1.z, localPos.z, instance.row1.w)));
    worldPos.z = fma(instance.row2.x, localPos.x, fma(instance.row2.y, localPos.y, fma(instance.row2.z, localPos.z, instance.row2.w)));

    gl_Position = camera.viewProj * vec4(worldPos, 1.0);

    vec3 N;
    N.x = dot(vec3(instance.row0.x, instance.row0.y, instance.row0.z), inNormal.xyz);
    N.y = dot(vec3(instance.row1.x, instance.row1.y, instance.row1.z), inNormal.xyz);
    N.z = dot(vec3(instance.row2.x, instance.row2.y, instance.row2.z), inNormal.xyz);
    N *= inversesqrt(dot(N, N));

    fragTexCoord = inTexCoord;
    fragNormal = N;
    fragColor = unpackUnorm4x8(instance.packedColor);
    fragTextureID = instance.textureID;
    fragWorldPos = worldPos;
}
