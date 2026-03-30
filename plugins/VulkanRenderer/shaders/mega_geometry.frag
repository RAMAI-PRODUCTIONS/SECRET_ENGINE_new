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
    // STEP 1: Test if shader is running - output RED
    // outColor = vec4(1.0, 0.0, 0.0, 1.0);
    
    // STEP 2: Test UV coordinates - should show gradient
    // outColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.0, 1.0);
    
    // STEP 3: Test texture sampling
    vec4 albedo = texture(diffuseTexture, fragTexCoord);
    
    // If texture is white (1,1,1), show MAGENTA to indicate problem
    if (albedo.r > 0.99 && albedo.g > 0.99 && albedo.b > 0.99) {
        outColor = vec4(1.0, 0.0, 1.0, 1.0); // MAGENTA = texture is white
    } else {
        outColor = albedo; // Show actual texture color
    }
    
    // STEP 4: Test if texture is completely black
    if (albedo.r < 0.01 && albedo.g < 0.01 && albedo.b < 0.01) {
        outColor = vec4(0.0, 1.0, 1.0, 1.0); // CYAN = texture is black
    }
}
