// ============================================================================
// SHADER DATA PACKING HELPERS
// Functions to prepare optimized vertex data for ultra-fast shaders
// ============================================================================

#pragma once
#include <cstdint>
#include <cmath>

namespace ShaderHelpers {

// ============================================================================
// NORMAL PACKING (12 bytes → 4 bytes)
// ============================================================================

/**
 * Pack a normalized vec3 normal into 10-10-10-2 format (uint32)
 * @param x, y, z - Normal components (will be normalized if not already)
 * @return Packed uint32 (4 bytes instead of 12!)
 */
inline uint32_t PackNormal(float x, float y, float z) {
    // Normalize input
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 0.0001f) {
        x /= len;
        y /= len;
        z /= len;
    }
    
    // Convert from [-1, 1] to [0, 1023] for 10-bit storage
    int32_t ix = static_cast<int32_t>((x + 1.0f) * 511.5f);
    int32_t iy = static_cast<int32_t>((y + 1.0f) * 511.5f);
    int32_t iz = static_cast<int32_t>((z + 1.0f) * 511.5f);
    
    // Clamp to 10-bit range
    ix = (ix < 0) ? 0 : (ix > 1023) ? 1023 : ix;
    iy = (iy < 0) ? 0 : (iy > 1023) ? 1023 : iy;
    iz = (iz < 0) ? 0 : (iz > 1023) ? 1023 : iz;
    
    // Pack into uint32: X in bits 0-9, Y in bits 10-19, Z in bits 20-29
    return static_cast<uint32_t>(
        (ix & 0x3FF) | 
        ((iy & 0x3FF) << 10) | 
        ((iz & 0x3FF) << 20)
    );
}

/**
 * Unpack a 10-10-10-2 normal back to vec3 (for debugging/testing)
 */
inline void UnpackNormal(uint32_t packed, float& x, float& y, float& z) {
    int32_t ix = (packed >>  0) & 0x3FF;
    int32_t iy = (packed >> 10) & 0x3FF;
    int32_t iz = (packed >> 20) & 0x3FF;
    
    x = (static_cast<float>(ix) / 511.5f) - 1.0f;
    y = (static_cast<float>(iy) / 511.5f) - 1.0f;
    z = (static_cast<float>(iz) / 511.5f) - 1.0f;
    
    // Renormalize
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 0.0001f) {
        x /= len;
        y /= len;
        z /= len;
    }
}

// ============================================================================
// HALF-PRECISION CONVERSION (4 bytes → 2 bytes)
// ============================================================================

/**
 * Convert float32 to float16 (IEEE 754 half-precision)
 * @param value - Float32 value
 * @return uint16_t - Packed float16
 */
inline uint16_t FloatToHalf(float value) {
    // Union for bit manipulation
    union FloatBits {
        float f;
        uint32_t u;
    };
    
    FloatBits bits;
    bits.f = value;
    
    uint32_t sign = (bits.u >> 16) & 0x8000;
    uint32_t exponent = ((bits.u >> 23) & 0xFF) - 112;
    uint32_t mantissa = (bits.u >> 13) & 0x3FF;
    
    // Handle special cases
    if (exponent <= 0) {
        // Zero or denormal
        if (exponent < -10) {
            return static_cast<uint16_t>(sign);  // Underflow to zero
        }
        // Denormal
        mantissa |= 0x400;
        mantissa >>= (1 - exponent);
        return static_cast<uint16_t>(sign | mantissa);
    } else if (exponent >= 31) {
        // Overflow to infinity
        return static_cast<uint16_t>(sign | 0x7C00);
    }
    
    // Normal case
    return static_cast<uint16_t>(sign | (exponent << 10) | mantissa);
}

/**
 * Convert float16 back to float32 (for debugging/testing)
 */
inline float HalfToFloat(uint16_t half) {
    uint32_t sign = (half & 0x8000) << 16;
    uint32_t exponent = (half & 0x7C00) >> 10;
    uint32_t mantissa = half & 0x3FF;
    
    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            return *reinterpret_cast<float*>(&sign);
        }
        // Denormal
        exponent = 1;
        while (!(mantissa & 0x400)) {
            mantissa <<= 1;
            exponent--;
        }
        mantissa &= 0x3FF;
    } else if (exponent == 31) {
        // Infinity or NaN
        return *reinterpret_cast<float*>(&(sign | 0x7F800000 | (mantissa << 13)));
    }
    
    // Normal case
    exponent += 112;
    uint32_t result = sign | (exponent << 23) | (mantissa << 13);
    return *reinterpret_cast<float*>(&result);
}

// ============================================================================
// CONVENIENCE FUNCTIONS FOR VERTEX DATA
// ============================================================================

/**
 * Pack a UV coordinate to half-precision
 */
inline void PackUV(float u, float v, uint16_t& outU, uint16_t& outV) {
    outU = FloatToHalf(u);
    outV = FloatToHalf(v);
}

/**
 * Pack a color to half-precision RGBA
 */
inline void PackColor(float r, float g, float b, float a, 
                     uint16_t& outR, uint16_t& outG, uint16_t& outB, uint16_t& outA) {
    outR = FloatToHalf(r);
    outG = FloatToHalf(g);
    outB = FloatToHalf(b);
    outA = FloatToHalf(a);
}

// ============================================================================
// EXAMPLE USAGE
// ============================================================================

/*
// Example: Packing a vertex for ultra-fast rendering

struct OptimizedVertex {
    float position[3];      // 12 bytes (keep full precision for geometry)
    uint32_t packedNormal;  // 4 bytes (was 12 bytes!)
    uint16_t uv[2];         // 4 bytes (was 8 bytes!)
};

void CreateOptimizedMesh(const std::vector<Vertex>& input, 
                        std::vector<OptimizedVertex>& output) {
    output.resize(input.size());
    
    for (size_t i = 0; i < input.size(); ++i) {
        const Vertex& in = input[i];
        OptimizedVertex& out = output[i];
        
        // Copy position (keep full precision)
        out.position[0] = in.position.x;
        out.position[1] = in.position.y;
        out.position[2] = in.position.z;
        
        // Pack normal (12 bytes → 4 bytes)
        out.packedNormal = PackNormal(in.normal.x, in.normal.y, in.normal.z);
        
        // Pack UV (8 bytes → 4 bytes)
        PackUV(in.uv.x, in.uv.y, out.uv[0], out.uv[1]);
    }
    
    // Result: 32 bytes/vertex → 20 bytes/vertex (37.5% smaller!)
}
*/

// ============================================================================
// QUALITY TESTING
// ============================================================================

/**
 * Test normal packing quality
 * @return Maximum error in degrees
 */
inline float TestNormalPackingQuality() {
    float maxError = 0.0f;
    
    // Test 1000 random normals
    for (int i = 0; i < 1000; ++i) {
        // Generate random normal
        float x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float z = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float len = sqrtf(x*x + y*y + z*z);
        x /= len; y /= len; z /= len;
        
        // Pack and unpack
        uint32_t packed = PackNormal(x, y, z);
        float ux, uy, uz;
        UnpackNormal(packed, ux, uy, uz);
        
        // Calculate error (dot product = cos(angle))
        float dot = x*ux + y*uy + z*uz;
        dot = (dot < -1.0f) ? -1.0f : (dot > 1.0f) ? 1.0f : dot;
        float errorDegrees = acosf(dot) * (180.0f / 3.14159265f);
        
        if (errorDegrees > maxError) {
            maxError = errorDegrees;
        }
    }
    
    return maxError;  // Typically < 0.5 degrees (imperceptible!)
}

/**
 * Test half-precision quality for typical game values
 * @return Maximum relative error percentage
 */
inline float TestHalfPrecisionQuality() {
    float maxError = 0.0f;
    
    // Test values in range [0, 1] (typical for UVs, colors)
    for (int i = 0; i <= 1000; ++i) {
        float value = i / 1000.0f;
        uint16_t packed = FloatToHalf(value);
        float unpacked = HalfToFloat(packed);
        
        float error = fabsf((unpacked - value) / value) * 100.0f;
        if (error > maxError) {
            maxError = error;
        }
    }
    
    return maxError;  // Typically < 0.1% (imperceptible!)
}

} // namespace ShaderHelpers

// ============================================================================
// INTEGRATION EXAMPLE
// ============================================================================

/*
#include "ShaderHelpers.h"

// In your mesh loading code:
struct Vertex {
    float position[3];
    uint32_t packedNormal;  // Use packed format!
    uint16_t uv[2];         // Use half-precision!
};

void LoadMesh(const char* path) {
    // ... load raw data ...
    
    std::vector<Vertex> vertices;
    for (auto& rawVertex : rawVertices) {
        Vertex v;
        v.position[0] = rawVertex.x;
        v.position[1] = rawVertex.y;
        v.position[2] = rawVertex.z;
        
        // Pack normal
        v.packedNormal = ShaderHelpers::PackNormal(
            rawVertex.nx, rawVertex.ny, rawVertex.nz);
        
        // Pack UV
        v.uv[0] = ShaderHelpers::FloatToHalf(rawVertex.u);
        v.uv[1] = ShaderHelpers::FloatToHalf(rawVertex.v);
        
        vertices.push_back(v);
    }
    
    // Create Vulkan buffer with packed data
    CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(Vertex));
}

// Run quality tests (in debug builds)
void ValidatePackingQuality() {
    float normalError = ShaderHelpers::TestNormalPackingQuality();
    float halfError = ShaderHelpers::TestHalfPrecisionQuality();
    
    printf("Normal packing error: %.3f degrees\n", normalError);
    printf("Half-precision error: %.3f%%\n", halfError);
    
    // Both should be imperceptible (<0.5 degrees, <0.1%)
}
*/
