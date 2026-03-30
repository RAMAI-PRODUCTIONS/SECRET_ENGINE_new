// SecretEngine
// Module: PhysicsPlugin
// Responsibility: Collision detection algorithms
// Dependencies: PhysicsTypes

#pragma once
#include "PhysicsTypes.h"
#include <SecretEngine/Components.h>
#include <cmath>
#include <algorithm>

namespace SecretEngine::Physics {

// Vector math helpers
namespace Math {
    inline float Dot(const float a[3], const float b[3]) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    }
    
    inline float Length(const float v[3]) {
        return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    }
    
    inline float LengthSq(const float v[3]) {
        return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    }
    
    inline void Normalize(const float v[3], float out[3]) {
        float len = Length(v);
        if (len > 0.0001f) {
            out[0] = v[0] / len;
            out[1] = v[1] / len;
            out[2] = v[2] / len;
        } else {
            out[0] = 0; out[1] = 0; out[2] = 0;
        }
    }
    
    inline void Sub(const float a[3], const float b[3], float out[3]) {
        out[0] = a[0] - b[0];
        out[1] = a[1] - b[1];
        out[2] = a[2] - b[2];
    }
    
    inline void Add(const float a[3], const float b[3], float out[3]) {
        out[0] = a[0] + b[0];
        out[1] = a[1] + b[1];
        out[2] = a[2] + b[2];
    }
    
    inline void Scale(const float v[3], float s, float out[3]) {
        out[0] = v[0] * s;
        out[1] = v[1] * s;
        out[2] = v[2] * s;
    }
    
    inline void ScaleAdd(const float a[3], const float b[3], float s, float out[3]) {
        out[0] = a[0] + b[0] * s;
        out[1] = a[1] + b[1] * s;
        out[2] = a[2] + b[2] * s;
    }
    
    inline float Clamp(float v, float min, float max) {
        return v < min ? min : (v > max ? max : v);
    }
}

// Collision detection functions
class CollisionDetection {
public:
    // Sphere vs Sphere
    static bool TestSphereSphere(
        const float posA[3], float radiusA,
        const float posB[3], float radiusB,
        CollisionInfo* outInfo = nullptr
    ) {
        float delta[3];
        Math::Sub(posB, posA, delta);
        float distSq = Math::LengthSq(delta);
        float radiusSum = radiusA + radiusB;
        
        if (distSq >= radiusSum * radiusSum) return false;
        
        if (outInfo) {
            float dist = std::sqrt(distSq);
            if (dist > 0.0001f) {
                Math::Scale(delta, 1.0f / dist, outInfo->normal);
            } else {
                outInfo->normal[0] = 0; outInfo->normal[1] = 1; outInfo->normal[2] = 0;
            }
            
            outInfo->penetration = radiusSum - dist;
            Math::ScaleAdd(posA, outInfo->normal, radiusA, outInfo->point);
        }
        
        return true;
    }
    
    // Box vs Box (AABB)
    static bool TestBoxBox(
        const float posA[3], const float extentsA[3],
        const float posB[3], const float extentsB[3],
        CollisionInfo* outInfo = nullptr
    ) {
        float delta[3];
        Math::Sub(posB, posA, delta);
        
        float overlap[3];
        for (int i = 0; i < 3; i++) {
            float totalExtent = extentsA[i] + extentsB[i];
            overlap[i] = totalExtent - std::abs(delta[i]);
            if (overlap[i] <= 0) return false;
        }
        
        if (outInfo) {
            // Find axis of minimum penetration
            int minAxis = 0;
            if (overlap[1] < overlap[minAxis]) minAxis = 1;
            if (overlap[2] < overlap[minAxis]) minAxis = 2;
            
            outInfo->penetration = overlap[minAxis];
            outInfo->normal[0] = outInfo->normal[1] = outInfo->normal[2] = 0;
            outInfo->normal[minAxis] = delta[minAxis] > 0 ? 1.0f : -1.0f;
            
            // Contact point
            for (int i = 0; i < 3; i++) {
                if (i == minAxis) {
                    outInfo->point[i] = posA[i] + outInfo->normal[i] * extentsA[i];
                } else {
                    outInfo->point[i] = (posA[i] + posB[i]) * 0.5f;
                }
            }
        }
        
        return true;
    }
    
    // Sphere vs Box
    static bool TestSphereBox(
        const float spherePos[3], float radius,
        const float boxPos[3], const float boxExtents[3],
        CollisionInfo* outInfo = nullptr
    ) {
        // Find closest point on box to sphere
        float closest[3];
        for (int i = 0; i < 3; i++) {
            closest[i] = Math::Clamp(spherePos[i], 
                                     boxPos[i] - boxExtents[i], 
                                     boxPos[i] + boxExtents[i]);
        }
        
        float delta[3];
        Math::Sub(spherePos, closest, delta);
        float distSq = Math::LengthSq(delta);
        
        if (distSq >= radius * radius) return false;
        
        if (outInfo) {
            float dist = std::sqrt(distSq);
            if (dist > 0.0001f) {
                Math::Scale(delta, 1.0f / dist, outInfo->normal);
            } else {
                // Sphere center inside box - find closest face
                float minDist = boxExtents[0] - std::abs(spherePos[0] - boxPos[0]);
                int minAxis = 0;
                for (int i = 1; i < 3; i++) {
                    float d = boxExtents[i] - std::abs(spherePos[i] - boxPos[i]);
                    if (d < minDist) {
                        minDist = d;
                        minAxis = i;
                    }
                }
                outInfo->normal[0] = outInfo->normal[1] = outInfo->normal[2] = 0;
                outInfo->normal[minAxis] = spherePos[minAxis] > boxPos[minAxis] ? 1.0f : -1.0f;
            }
            
            outInfo->penetration = radius - dist;
            for (int i = 0; i < 3; i++) {
                outInfo->point[i] = closest[i];
            }
        }
        
        return true;
    }
    
    // Capsule vs Capsule (simplified)
    static bool TestCapsuleCapsule(
        const float posA[3], float radiusA, float halfHeightA,
        const float posB[3], float radiusB, float halfHeightB,
        CollisionInfo* outInfo = nullptr
    ) {
        // Treat as sphere-sphere for now (simplified)
        float effectiveRadiusA = radiusA + halfHeightA;
        float effectiveRadiusB = radiusB + halfHeightB;
        return TestSphereSphere(posA, effectiveRadiusA, posB, effectiveRadiusB, outInfo);
    }
    
    // Ray vs Sphere
    static bool RaySphere(
        const float rayOrigin[3], const float rayDir[3],
        const float spherePos[3], float radius,
        float& outDistance
    ) {
        float oc[3];
        Math::Sub(rayOrigin, spherePos, oc);
        
        float a = Math::Dot(rayDir, rayDir);
        float b = 2.0f * Math::Dot(oc, rayDir);
        float c = Math::Dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return false;
        
        outDistance = (-b - std::sqrt(discriminant)) / (2.0f * a);
        return outDistance >= 0;
    }
    
    // Ray vs Box (AABB)
    static bool RayBox(
        const float rayOrigin[3], const float rayDir[3],
        const float boxPos[3], const float boxExtents[3],
        float& outDistance
    ) {
        float tMin = 0.0f;
        float tMax = 1e30f;
        
        for (int i = 0; i < 3; i++) {
            if (std::abs(rayDir[i]) < 0.0001f) {
                if (rayOrigin[i] < boxPos[i] - boxExtents[i] || 
                    rayOrigin[i] > boxPos[i] + boxExtents[i]) {
                    return false;
                }
            } else {
                float invD = 1.0f / rayDir[i];
                float t1 = (boxPos[i] - boxExtents[i] - rayOrigin[i]) * invD;
                float t2 = (boxPos[i] + boxExtents[i] - rayOrigin[i]) * invD;
                
                if (t1 > t2) std::swap(t1, t2);
                
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                
                if (tMin > tMax) return false;
            }
        }
        
        outDistance = tMin;
        return true;
    }
};

} // namespace SecretEngine::Physics
