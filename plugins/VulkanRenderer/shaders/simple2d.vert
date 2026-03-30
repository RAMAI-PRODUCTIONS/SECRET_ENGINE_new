#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 offset;
} pushConstants;

void main() {
    // Transform from pixel coordinates to NDC (-1 to 1)
    gl_Position = vec4(inPosition * pushConstants.scale + pushConstants.offset, 0.0, 1.0);
    fragColor = inColor;
}
