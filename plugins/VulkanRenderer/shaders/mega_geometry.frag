#version 450

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;         // Instance color (base tint)
layout(location = 3) in vec3 fragVertexLight;   // Dynamic per-vertex lighting
layout(location = 4) in flat uint fragTextureID;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // NO TEXTURES - Pure vertex color rendering
    // Combine instance color (tint) with dynamic vertex lighting
    vec3 finalColor = fragColor.rgb * fragVertexLight;
    
    // Add small ambient term to prevent pure black
    finalColor += fragColor.rgb * 0.05;
    
    // Optional: Tonemap for HDR (simple Reinhard)
    finalColor = finalColor / (finalColor + vec3(1.0));
    
    outColor = vec4(finalColor, fragColor.a);
}
