#pragma once
#include <cstdint>
#include <vector>
#include <cmath>
#include <SecretEngine/Math.h>

namespace SecretEngine {

using namespace SecretEngine::Math;

// ============================================================================
// SIMPLE VERTEX LIGHTING SYSTEM
// No textures, no shadow maps - just pure vertex colors with dynamic lighting
// ============================================================================

struct Light {
    enum Type { Directional = 0, Point = 1, Spot = 2 };
    
    Type type;
    Float3 position;
    Float3 direction;
    Float3 color;
    float intensity;
    float range;
    float spotAngle;
};

// Helper functions for Float3
inline Float3 operator+(const Float3& a, const Float3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Float3 operator-(const Float3& a, const Float3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Float3 operator-(const Float3& a) {
    return {-a.x, -a.y, -a.z};
}

inline Float3 operator*(const Float3& a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}

inline Float3 operator/(const Float3& a, float s) {
    return {a.x / s, a.y / s, a.z / s};
}

inline float Dot(const Float3& a, const Float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float Length(const Float3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Float3 Normalize(const Float3& v) {
    float len = Length(v);
    return (len > 0.0f) ? (v / len) : Float3{0.0f, 0.0f, 0.0f};
}

inline float Max(float a, float b) {
    return (a > b) ? a : b;
}

inline float Clamp(float v, float min, float max) {
    return (v < min) ? min : ((v > max) ? max : v);
}

inline float Smoothstep(float edge0, float edge1, float x) {
    float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

class SimpleVertexLighting {
public:
    SimpleVertexLighting() = default;
    
    // Add lights to the scene
    void AddDirectionalLight(const Float3& direction, const Float3& color, float intensity) {
        Light light;
        light.type = Light::Directional;
        light.direction = Normalize(direction);
        light.color = color;
        light.intensity = intensity;
        m_lights.push_back(light);
    }
    
    void AddPointLight(const Float3& position, const Float3& color, float intensity, float range) {
        Light light;
        light.type = Light::Point;
        light.position = position;
        light.color = color;
        light.intensity = intensity;
        light.range = range;
        m_lights.push_back(light);
    }
    
    void AddSpotLight(const Float3& position, const Float3& direction, 
                      const Float3& color, float intensity, float range, float spotAngle) {
        Light light;
        light.type = Light::Spot;
        light.position = position;
        light.direction = Normalize(direction);
        light.color = color;
        light.intensity = intensity;
        light.range = range;
        light.spotAngle = spotAngle;
        m_lights.push_back(light);
    }
    
    void ClearLights() {
        m_lights.clear();
    }
    
    // Compute lighting for a single vertex
    Float3 ComputeVertexLighting(const Float3& position, const Float3& normal) const {
        Float3 lighting{0.0f, 0.0f, 0.0f};
        
        // Ambient term
        lighting = lighting + (m_ambientColor * m_ambientIntensity);
        
        // Process each light
        for (const auto& light : m_lights) {
            switch (light.type) {
                case Light::Directional:
                    lighting = lighting + ComputeDirectional(light, normal);
                    break;
                case Light::Point:
                    lighting = lighting + ComputePoint(light, position, normal);
                    break;
                case Light::Spot:
                    lighting = lighting + ComputeSpot(light, position, normal);
                    break;
            }
        }
        
        return lighting;
    }
    
    // Compute lighting for all vertices in a mesh
    void ComputeMeshLighting(const Float3* positions, const Float3* normals, 
                            uint32_t* outColors, uint32_t vertexCount) const {
        for (uint32_t i = 0; i < vertexCount; ++i) {
            Float3 lighting = ComputeVertexLighting(positions[i], normals[i]);
            outColors[i] = PackR11G11B10F(lighting);
        }
    }
    
    // Pack RGB to R11G11B10F format
    static uint32_t PackR11G11B10F(const Float3& color) {
        // Clamp to [0, 64] range (HDR)
        float r = Clamp(color.x, 0.0f, 64.0f);
        float g = Clamp(color.y, 0.0f, 64.0f);
        float b = Clamp(color.z, 0.0f, 64.0f);
        
        // Normalize to [0, 1]
        r = r / 64.0f;
        g = g / 64.0f;
        b = b / 64.0f;
        
        // Convert to integer
        uint32_t ri = static_cast<uint32_t>(r * 2047.0f + 0.5f); // 11 bits
        uint32_t gi = static_cast<uint32_t>(g * 2047.0f + 0.5f); // 11 bits
        uint32_t bi = static_cast<uint32_t>(b * 1023.0f + 0.5f); // 10 bits
        
        // Pack into 32-bit uint
        return (ri << 21) | (gi << 10) | bi;
    }
    
    // Unpack R11G11B10F to RGB (for debugging)
    static Float3 UnpackR11G11B10F(uint32_t packed) {
        uint32_t r = (packed >> 21) & 0x7FF;
        uint32_t g = (packed >> 10) & 0x7FF;
        uint32_t b = packed & 0x3FF;
        
        return Float3{
            float(r) / 2047.0f * 64.0f,
            float(g) / 2047.0f * 64.0f,
            float(b) / 1023.0f * 64.0f
        };
    }
    
    // Set ambient lighting
    void SetAmbient(const Float3& color, float intensity) {
        m_ambientColor = color;
        m_ambientIntensity = intensity;
    }
    
    // Get lights (for shadow/GI system)
    const std::vector<Light>& GetLights() const { return m_lights; }
    
    // Get ambient (for shadow/GI system)
    Float3 GetAmbient() const { return m_ambientColor * m_ambientIntensity; }
    
private:
    std::vector<Light> m_lights;
    Float3 m_ambientColor = Float3{0.2f, 0.2f, 0.3f};
    float m_ambientIntensity = 0.1f;
    
    Float3 ComputeDirectional(const Light& light, const Float3& normal) const {
        float NdotL = Max(0.0f, Dot(normal, -light.direction));
        return light.color * light.intensity * NdotL;
    }
    
    Float3 ComputePoint(const Light& light, const Float3& position, const Float3& normal) const {
        Float3 toLight = light.position - position;
        float distance = Length(toLight);
        
        if (distance > light.range) return Float3{0.0f, 0.0f, 0.0f};
        
        Float3 lightDir = toLight / distance;
        float NdotL = Max(0.0f, Dot(normal, lightDir));
        
        // Inverse square falloff with range limit
        float attenuation = 1.0f / (1.0f + distance * distance / (light.range * light.range));
        
        return light.color * light.intensity * NdotL * attenuation;
    }
    
    Float3 ComputeSpot(const Light& light, const Float3& position, const Float3& normal) const {
        Float3 toLight = light.position - position;
        float distance = Length(toLight);
        
        if (distance > light.range) return Float3{0.0f, 0.0f, 0.0f};
        
        Float3 lightDir = toLight / distance;
        float NdotL = Max(0.0f, Dot(normal, lightDir));
        
        // Spot cone
        float spotEffect = Dot(lightDir, -light.direction);
        float spotCutoff = std::cos(light.spotAngle);
        
        if (spotEffect < spotCutoff) return Float3{0.0f, 0.0f, 0.0f};
        
        // Smooth falloff at cone edge
        float spotAttenuation = Smoothstep(spotCutoff, 1.0f, spotEffect);
        
        // Distance attenuation
        float attenuation = 1.0f / (1.0f + distance * distance / (light.range * light.range));
        
        return light.color * light.intensity * NdotL * attenuation * spotAttenuation;
    }
};

} // namespace SecretEngine
