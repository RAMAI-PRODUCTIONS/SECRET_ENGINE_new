#pragma once
#include <cstdint>
#include <vector>
#include <cmath>
#include <SecretEngine/Math.h>
#include "SimpleVertexLighting.h"

namespace SecretEngine {

using namespace SecretEngine::Math;

// ============================================================================
// VERTEX SHADOWS & GLOBAL ILLUMINATION
// Adds shadow raycasting and hemisphere GI to vertex lighting
// ============================================================================

struct SceneGeometry {
    std::vector<Float3> positions;
    std::vector<Float3> normals;
    std::vector<uint32_t> indices;
    
    void Clear() {
        positions.clear();
        normals.clear();
        indices.clear();
    }
    
    void AddMesh(const Float3* verts, const Float3* norms, uint32_t vertCount,
                 const uint32_t* inds, uint32_t indCount) {
        uint32_t baseVertex = positions.size();
        
        for (uint32_t i = 0; i < vertCount; ++i) {
            positions.push_back(verts[i]);
            normals.push_back(norms[i]);
        }
        
        for (uint32_t i = 0; i < indCount; ++i) {
            indices.push_back(baseVertex + inds[i]);
        }
    }
};

class VertexShadowsGI {
public:
    VertexShadowsGI() = default;
    
    // Configure GI settings
    void SetSkyColor(const Float3& color) { m_skyColor = color; }
    void SetGroundColor(const Float3& color) { m_groundColor = color; }
    void SetGIIntensity(float intensity) { m_giIntensity = intensity; }
    void SetShadowSoftness(float softness) { m_shadowSoftness = softness; }
    
    // Ray-triangle intersection for shadow testing
    bool RayTriangleIntersect(const Float3& origin, const Float3& dir,
                             const Float3& v0, const Float3& v1, const Float3& v2,
                             float& t) const {
        const float EPSILON = 0.0000001f;
        Float3 edge1 = v1 - v0;
        Float3 edge2 = v2 - v0;
        Float3 h = Cross(dir, edge2);
        float a = Dot(edge1, h);
        
        if (a > -EPSILON && a < EPSILON) return false;
        
        float f = 1.0f / a;
        Float3 s = origin - v0;
        float u = f * Dot(s, h);
        
        if (u < 0.0f || u > 1.0f) return false;
        
        Float3 q = Cross(s, edge1);
        float v = f * Dot(dir, q);
        
        if (v < 0.0f || u + v > 1.0f) return false;
        
        t = f * Dot(edge2, q);
        return t > EPSILON;
    }
    
    // Test if vertex is in shadow from a light
    bool IsInShadow(const Float3& position, const Float3& lightDir, float maxDist,
                   const SceneGeometry& scene) const {
        // Offset start position slightly to avoid self-intersection
        Float3 origin = position + lightDir * 0.01f;
        
        // Test against all triangles
        for (size_t i = 0; i < scene.indices.size(); i += 3) {
            Float3 v0 = scene.positions[scene.indices[i]];
            Float3 v1 = scene.positions[scene.indices[i + 1]];
            Float3 v2 = scene.positions[scene.indices[i + 2]];
            
            float t;
            if (RayTriangleIntersect(origin, lightDir, v0, v1, v2, t)) {
                if (t < maxDist) {
                    return true; // Hit something before reaching light
                }
            }
        }
        
        return false; // No occlusion
    }
    
    // Compute hemisphere GI (sky + ground bounce)
    Float3 ComputeHemisphereGI(const Float3& normal) const {
        // Sky contribution (from above)
        float skyFactor = Max(0.0f, normal.z); // Z is up
        Float3 skyContrib = m_skyColor * skyFactor;
        
        // Ground contribution (from below)
        float groundFactor = Max(0.0f, -normal.z);
        Float3 groundContrib = m_groundColor * groundFactor;
        
        return (skyContrib + groundContrib) * m_giIntensity;
    }
    
    // Compute full lighting with shadows and GI
    Float3 ComputeLightingWithShadowsGI(const Float3& position, const Float3& normal,
                                        const SimpleVertexLighting& lighting,
                                        const SceneGeometry& scene,
                                        const Float3& materialDiffuse) const {
        Float3 result{0.0f, 0.0f, 0.0f};
        
        // 1. Ambient + GI
        Float3 ambient = lighting.GetAmbient();
        Float3 gi = ComputeHemisphereGI(normal);
        result = result + ambient + gi;
        
        // 2. Direct lighting with shadows
        const auto& lights = lighting.GetLights();
        for (const auto& light : lights) {
            Float3 lightContrib{0.0f, 0.0f, 0.0f};
            float shadowFactor = 1.0f;
            
            switch (light.type) {
                case Light::Directional: {
                    Float3 lightDir = -light.direction;
                    float NdotL = Max(0.0f, Dot(normal, lightDir));
                    
                    if (NdotL > 0.0f) {
                        // Shadow test (infinite distance for directional)
                        if (IsInShadow(position, lightDir, 10000.0f, scene)) {
                            shadowFactor = m_shadowSoftness;
                        }
                        lightContrib = light.color * light.intensity * NdotL * shadowFactor;
                    }
                    break;
                }
                
                case Light::Point: {
                    Float3 toLight = light.position - position;
                    float distance = Length(toLight);
                    
                    if (distance < light.range) {
                        Float3 lightDir = toLight / distance;
                        float NdotL = Max(0.0f, Dot(normal, lightDir));
                        
                        if (NdotL > 0.0f) {
                            // Shadow test
                            if (IsInShadow(position, lightDir, distance, scene)) {
                                shadowFactor = m_shadowSoftness;
                            }
                            
                            float attenuation = 1.0f / (1.0f + distance * distance / (light.range * light.range));
                            lightContrib = light.color * light.intensity * NdotL * attenuation * shadowFactor;
                        }
                    }
                    break;
                }
                
                case Light::Spot: {
                    Float3 toLight = light.position - position;
                    float distance = Length(toLight);
                    
                    if (distance < light.range) {
                        Float3 lightDir = toLight / distance;
                        float NdotL = Max(0.0f, Dot(normal, lightDir));
                        
                        // Spot cone test
                        float spotEffect = Dot(lightDir, -light.direction);
                        float spotCutoff = std::cos(light.spotAngle);
                        
                        if (spotEffect > spotCutoff && NdotL > 0.0f) {
                            // Shadow test
                            if (IsInShadow(position, lightDir, distance, scene)) {
                                shadowFactor = m_shadowSoftness;
                            }
                            
                            float spotAttenuation = Smoothstep(spotCutoff, 1.0f, spotEffect);
                            float attenuation = 1.0f / (1.0f + distance * distance / (light.range * light.range));
                            lightContrib = light.color * light.intensity * NdotL * attenuation * spotAttenuation * shadowFactor;
                        }
                    }
                    break;
                }
            }
            
            result = result + lightContrib;
        }
        
        // 3. Modulate by material diffuse color
        result = Float3{result.x * materialDiffuse.x, result.y * materialDiffuse.y, result.z * materialDiffuse.z};
        
        return result;
    }
    
    // Helper: Cross product
    static Float3 Cross(const Float3& a, const Float3& b) {
        return Float3{
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
    
private:
    Float3 m_skyColor = Float3{0.5f, 0.7f, 1.0f};      // Blue sky
    Float3 m_groundColor = Float3{0.3f, 0.25f, 0.2f};  // Brown ground
    float m_giIntensity = 0.3f;
    float m_shadowSoftness = 0.2f; // 0 = hard shadows, 1 = no shadows
};

} // namespace SecretEngine
