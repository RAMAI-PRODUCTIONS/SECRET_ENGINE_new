// MEGA GEOMETRY FRAGMENT SHADER - Dynamic lighting
#version 450

layout(location = 0) in vec2  fragTexCoord;
layout(location = 1) in vec3  fragNormal;
layout(location = 2) in vec4  fragColor;
layout(location = 3) in flat uint fragTextureID;
layout(location = 4) in vec3  fragWorldPos;

layout(location = 0) out vec4 outColor;

// Light data SSBO - individual floats to match C++ struct packing (no vec3 alignment)
// C++ LightData is 68 bytes: type(4) + pos(12) + dir(12) + col(12) + intensity(4) +
//   range(4) + spotAngle(4) + constAtten(4) + linearAtten(4) + quadAtten(4) + pad(4)
struct LightData {
    int   type;
    float posX, posY, posZ;
    float dirX, dirY, dirZ;
    float colR, colG, colB;
    float intensity;
    float range;
    float spotAngle;
    float constAtten;
    float linearAtten;
    float quadAtten;
    uint  padding;
};

layout(std430, set = 1, binding = 0) readonly buffer LightBuffer {
    LightData lights[];
};

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    uint lightCount;
} pc;

void main() {
    vec3 albedo = fragColor.rgb;
    vec3 N = normalize(fragNormal);

    // Ambient
    vec3 finalColor = albedo * 0.08;

    // Dynamic point/directional lights
    uint numLights = pc.lightCount;
    for (uint i = 0u; i < numLights; i++) {
        LightData L = lights[i];
        vec3 lightColor = vec3(L.colR, L.colG, L.colB) * L.intensity;
        vec3 lightDir;
        float atten = 1.0;

        if (L.type == 0) {
            // Directional
            lightDir = normalize(-vec3(L.dirX, L.dirY, L.dirZ));
        } else {
            // Point / Spot
            vec3 toLight = vec3(L.posX, L.posY, L.posZ) - fragWorldPos;
            float dist   = length(toLight);
            if (dist > L.range) continue;
            lightDir = toLight / dist;
            atten = 1.0 / (L.constAtten + L.linearAtten * dist + L.quadAtten * dist * dist);

            if (L.type == 2) {
                float cosA = dot(-lightDir, normalize(vec3(L.dirX, L.dirY, L.dirZ)));
                float cosS = cos(L.spotAngle * 0.5);
                if (cosA < cosS) continue;
                atten *= smoothstep(cosS, cosS + 0.05, cosA);
            }
        }

        float diff = max(dot(N, lightDir), 0.0);
        finalColor += albedo * lightColor * diff * atten;
    }

    // Fallback sun when no dynamic lights
    if (numLights == 0u) {
        vec3 sun = normalize(vec3(0.5, 1.0, 0.3));
        float diff = max(dot(N, sun), 0.0);
        finalColor = albedo * (0.15 + diff * 0.85);
    }

    outColor = vec4(finalColor, fragColor.a);
}
