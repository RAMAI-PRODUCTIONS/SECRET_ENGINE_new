#version 450

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragVertexLight;  // Vertex GI (R11G11B10F unpacked)

// Texture samplers (2 slots: diffuse + normal map)
layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // Sample albedo texture
    vec4 albedo = texture(diffuseTexture, fragTexCoord);
    
    // Apply vertex GI lighting
    // fragVertexLight contains pre-computed global illumination
    vec3 finalColor = albedo.rgb * fragVertexLight;
    
    // Optional: Add small ambient term to prevent pure black
    finalColor += albedo.rgb * 0.03;
    
    // Optional: Tonemap for HDR (simple Reinhard)
    // finalColor = finalColor / (finalColor + vec3(1.0));
    
    outColor = vec4(finalColor, albedo.a);
}
