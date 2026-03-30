#version 450

layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Solid color (no lighting) to debug visibility
    outColor = inColor;
}
