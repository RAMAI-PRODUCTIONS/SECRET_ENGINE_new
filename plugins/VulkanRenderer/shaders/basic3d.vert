#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// Instance attributes (128-byte Nitrogen Layout) -> Reverted to 64-byte
layout(location = 3) in vec4 instRow0;
layout(location = 4) in vec4 instRow1;
layout(location = 5) in vec4 instRow2;
layout(location = 6) in uint packedColor;

layout(location = 2) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
} pc;

void main() {
    vec4 localPos = vec4(inPosition, 1.0);
    
    // NITRO BALANCED MATH
    vec3 worldPos;
    worldPos.x = dot(instRow0, localPos);
    worldPos.y = dot(instRow1, localPos);
    worldPos.z = dot(instRow2, localPos);

    gl_Position = pc.viewProj * vec4(worldPos, 1.0);
    
    // Normal Transformation (for future use, but not used for lighting)
    vec3 N;
    N.x = dot(instRow0.xyz, inNormal);
    N.y = dot(instRow1.xyz, inNormal);
    N.z = dot(instRow2.xyz, inNormal);
    N = normalize(N);

    // UNLIT: No lighting calculations - use raw instance color
    vec4 instanceColor = unpackUnorm4x8(packedColor);
    outColor = instanceColor;  // UNLIT: Direct color, no lighting multiplier
}
