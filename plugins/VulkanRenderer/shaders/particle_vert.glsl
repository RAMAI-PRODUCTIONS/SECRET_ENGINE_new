#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 viewProj;
    vec3 cameraPos;
} camera;

void main() {
    gl_Position = camera.viewProj * vec4(inPosition, 1.0);
    gl_PointSize = 3.0; // Particle size
    fragColor = inColor;
}
