#version 450

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;

// Texture samplers (2 slots: diffuse + normal map)
layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // Sample texture once (cached by GPU)
    vec4 albedo = texture(diffuseTexture, fragTexCoord);
    
    // Multiply by per-instance color (from material_params in JSON)
    // This is still very fast - single texture sample + multiply
    outColor = albedo * fragColor;
}
