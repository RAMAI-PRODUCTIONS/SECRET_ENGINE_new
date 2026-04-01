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

// Overlay blend mode: preserves texture detail while applying color
// Formula: base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend))
vec3 overlay(vec3 base, vec3 blend) {
    return vec3(
        base.r < 0.5 ? (2.0 * base.r * blend.r) : (1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r)),
        base.g < 0.5 ? (2.0 * base.g * blend.g) : (1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g)),
        base.b < 0.5 ? (2.0 * base.b * blend.b) : (1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b))
    );
}

void main() {
    // Sample texture once (cached by GPU)
    vec4 albedo = texture(diffuseTexture, fragTexCoord);
    
    // Apply overlay blend with per-instance color (from material_params in JSON)
    // This preserves texture detail while applying the color tint
    vec3 blended = overlay(albedo.rgb, fragColor.rgb);
    
    outColor = vec4(blended, albedo.a);
}
