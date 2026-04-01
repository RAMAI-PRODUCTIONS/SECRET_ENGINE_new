#version 450

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // ULTRA-FAST: Use only per-instance color (no texture sampling)
    // Each instance gets unique color from material_params in JSON
    // This is the fastest possible fragment shader - single assignment
    outColor = fragColor;
}
