#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in flat uint fragTextureID;
layout(location = 4) in vec3 fragWorldPos;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // Use instance color as albedo (no texture dependency)
    vec3 albedo = fragColor.rgb;

    vec3 N = normalize(fragNormal);

    // Simple directional sun light
    vec3 sunDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(N, sunDir), 0.0);
    vec3 finalColor = albedo * (0.15 + diff * 0.85);

    outColor = vec4(finalColor, fragColor.a);
}
