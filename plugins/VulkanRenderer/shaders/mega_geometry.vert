// MEGA GEOMETRY VERTEX SHADER
#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 inPosition; // R16G16B16A16_SNORM
layout(location = 1) in vec4 inNormal;   // R8G8B8A8_SNORM
layout(location = 2) in vec2 inTexCoord; // R16G16_UNORM

layout(location = 0) out vec2  fragTexCoord;
layout(location = 1) out vec3  fragNormal;
layout(location = 2) out vec4  fragColor;
layout(location = 3) out flat uint fragTextureID;
layout(location = 4) out vec3  fragWorldPos;

struct InstanceData {
    vec4 row0;
    vec4 row1;
    vec4 row2;
    uint packedColor;
    uint textureID;
    uint _pad0;
    uint _pad1;
};

layout(std430, set = 0, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(push_constant) uniform PushConstants {
    mat4  viewProj;
    uint  lightCount;
} pc;

void main() {
    InstanceData inst = instances[gl_InstanceIndex];

    vec3 localPos = inPosition.xyz * 8.0;

    vec3 worldPos;
    worldPos.x = fma(inst.row0.x, localPos.x, fma(inst.row0.y, localPos.y, fma(inst.row0.z, localPos.z, inst.row0.w)));
    worldPos.y = fma(inst.row1.x, localPos.x, fma(inst.row1.y, localPos.y, fma(inst.row1.z, localPos.z, inst.row1.w)));
    worldPos.z = fma(inst.row2.x, localPos.x, fma(inst.row2.y, localPos.y, fma(inst.row2.z, localPos.z, inst.row2.w)));

    gl_Position = pc.viewProj * vec4(worldPos, 1.0);

    vec3 N;
    N.x = dot(vec3(inst.row0.x, inst.row0.y, inst.row0.z), inNormal.xyz);
    N.y = dot(vec3(inst.row1.x, inst.row1.y, inst.row1.z), inNormal.xyz);
    N.z = dot(vec3(inst.row2.x, inst.row2.y, inst.row2.z), inNormal.xyz);
    N *= inversesqrt(max(dot(N, N), 0.0001));

    fragTexCoord  = inTexCoord;
    fragNormal    = N;
    fragColor     = unpackUnorm4x8(inst.packedColor);
    fragTextureID = inst.textureID;
    fragWorldPos  = worldPos;
}
