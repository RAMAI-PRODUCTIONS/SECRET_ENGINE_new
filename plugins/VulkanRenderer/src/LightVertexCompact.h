#pragma once

#include <cstdint>
#include <cmath>
#include <glm/glm.hpp>

/**
 * LightVertexCompact - 24-byte compact light vertex format
 * 
 * Optimized for GPU cache efficiency and SIMD processing.
 * Designed for vertex-based global illumination with per-instance updates.
 * 
 * Memory layout (24 bytes total):
 * - Position: 6 bytes (quantized to 16-bit per axis)
 * - Radiance: 4 bytes (RGBE format)
 * - Normal: 2 bytes (octahedral encoding)
 * - Flags: 2 bytes (material properties, LOD, etc.)
 * - Radius/PDF: 4 bytes (light influence radius + probability density)
 * - Instance ID: 2 bytes
 * - Padding: 4 bytes (for 8-byte alignment)
 */
struct LightVertexCompact {
    // Position quantized to 16-bit per axis (6 bytes)
    // Represents position in local space [-32768, 32767] mapped to world bounds
    uint16_t posX;
    uint16_t posY;
    uint16_t posZ;
    
    // Radiance in RGBE format (4 bytes)
    // RGB mantissa (8-bit each) + shared exponent (8-bit)
    uint8_t rgbE[4];
    
    // Normal in octahedral encoding (2 bytes)
    // Encodes unit normal vector into 2 bytes with minimal error
    uint16_t normalPacked;
    
    // Material flags and properties (2 bytes)
    // Bits 0-3: Material type (0=diffuse, 1=metal, 2=glass, etc.)
    // Bits 4-7: LOD level (0-15)
    // Bits 8-11: Emissive flag
    // Bits 12-15: Reserved
    uint16_t flags;
    
    // Light influence radius (16-bit float) + PDF (16-bit float) (4 bytes)
    // Radius: how far this light vertex affects neighbors
    // PDF: probability density for importance sampling
    uint32_t radiusPdf;
    
    // Instance ID (2 bytes) - which mesh instance this vertex belongs to
    uint16_t instanceId;
    
    // Padding for 8-byte alignment (4 bytes)
    uint32_t padding;
    
    // --- Encoding Functions ---
    
    /**
     * Encode position from world space to quantized 16-bit format
     * @param worldPos World space position
     * @param boundsMin Minimum bounds of the scene
     * @param boundsMax Maximum bounds of the scene
     */
    static void encodePosition(LightVertexCompact& vertex, 
                               const glm::vec3& worldPos,
                               const glm::vec3& boundsMin,
                               const glm::vec3& boundsMax) {
        glm::vec3 normalized = (worldPos - boundsMin) / (boundsMax - boundsMin);
        vertex.posX = static_cast<uint16_t>(glm::clamp(normalized.x, 0.0f, 1.0f) * 65535.0f);
        vertex.posY = static_cast<uint16_t>(glm::clamp(normalized.y, 0.0f, 1.0f) * 65535.0f);
        vertex.posZ = static_cast<uint16_t>(glm::clamp(normalized.z, 0.0f, 1.0f) * 65535.0f);
    }
    
    /**
     * Decode position from quantized format to world space
     */
    static glm::vec3 decodePosition(const LightVertexCompact& vertex,
                                    const glm::vec3& boundsMin,
                                    const glm::vec3& boundsMax) {
        glm::vec3 normalized(
            vertex.posX / 65535.0f,
            vertex.posY / 65535.0f,
            vertex.posZ / 65535.0f
        );
        return boundsMin + normalized * (boundsMax - boundsMin);
    }
    
    /**
     * Encode RGB color to RGBE format (Radiance HDR format)
     * Allows storing HDR colors in 4 bytes with shared exponent
     */
    static void encodeRGBE(LightVertexCompact& vertex, const glm::vec3& rgb) {
        float maxComponent = glm::max(glm::max(rgb.r, rgb.g), rgb.b);
        
        if (maxComponent < 1e-32f) {
            vertex.rgbE[0] = vertex.rgbE[1] = vertex.rgbE[2] = vertex.rgbE[3] = 0;
            return;
        }
        
        int exponent;
        float mantissa = std::frexp(maxComponent, &exponent) * 256.0f / maxComponent;
        
        vertex.rgbE[0] = static_cast<uint8_t>(rgb.r * mantissa);
        vertex.rgbE[1] = static_cast<uint8_t>(rgb.g * mantissa);
        vertex.rgbE[2] = static_cast<uint8_t>(rgb.b * mantissa);
        vertex.rgbE[3] = static_cast<uint8_t>(exponent + 128);
    }
    
    /**
     * Decode RGBE to RGB float
     */
    static glm::vec3 decodeRGBE(const LightVertexCompact& vertex) {
        if (vertex.rgbE[3] == 0) {
            return glm::vec3(0.0f);
        }
        
        float exponent = std::ldexp(1.0f, static_cast<int>(vertex.rgbE[3]) - 128 - 8);
        return glm::vec3(
            vertex.rgbE[0] * exponent,
            vertex.rgbE[1] * exponent,
            vertex.rgbE[2] * exponent
        );
    }
    
    /**
     * Encode normal to octahedral format (2 bytes)
     * Maps sphere to square with minimal distortion
     */
    static void encodeNormal(LightVertexCompact& vertex, const glm::vec3& normal) {
        glm::vec3 n = glm::normalize(normal);
        
        // Project to octahedron
        n /= (std::abs(n.x) + std::abs(n.y) + std::abs(n.z));
        
        glm::vec2 octWrap;
        if (n.z < 0.0f) {
            octWrap.x = (1.0f - std::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
            octWrap.y = (1.0f - std::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
        } else {
            octWrap.x = n.x;
            octWrap.y = n.y;
        }
        
        // Pack to 16-bit
        uint16_t x = static_cast<uint16_t>((octWrap.x * 0.5f + 0.5f) * 255.0f);
        uint16_t y = static_cast<uint16_t>((octWrap.y * 0.5f + 0.5f) * 255.0f);
        vertex.normalPacked = (x << 8) | y;
    }
    
    /**
     * Decode octahedral normal to vec3
     */
    static glm::vec3 decodeNormal(const LightVertexCompact& vertex) {
        uint16_t x = (vertex.normalPacked >> 8) & 0xFF;
        uint16_t y = vertex.normalPacked & 0xFF;
        
        glm::vec2 oct(
            (x / 255.0f) * 2.0f - 1.0f,
            (y / 255.0f) * 2.0f - 1.0f
        );
        
        glm::vec3 n(oct.x, oct.y, 1.0f - std::abs(oct.x) - std::abs(oct.y));
        
        if (n.z < 0.0f) {
            float oldX = n.x;
            n.x = (1.0f - std::abs(n.y)) * (oldX >= 0.0f ? 1.0f : -1.0f);
            n.y = (1.0f - std::abs(oldX)) * (n.y >= 0.0f ? 1.0f : -1.0f);
        }
        
        return glm::normalize(n);
    }
    
    /**
     * Pack radius and PDF into single uint32
     * Each gets 16 bits (half-float precision)
     */
    static void encodeRadiusPdf(LightVertexCompact& vertex, float radius, float pdf) {
        uint16_t radiusHalf = floatToHalf(radius);
        uint16_t pdfHalf = floatToHalf(pdf);
        vertex.radiusPdf = (static_cast<uint32_t>(radiusHalf) << 16) | pdfHalf;
    }
    
    /**
     * Unpack radius and PDF
     */
    static void decodeRadiusPdf(const LightVertexCompact& vertex, float& radius, float& pdf) {
        uint16_t radiusHalf = (vertex.radiusPdf >> 16) & 0xFFFF;
        uint16_t pdfHalf = vertex.radiusPdf & 0xFFFF;
        radius = halfToFloat(radiusHalf);
        pdf = halfToFloat(pdfHalf);
    }
    
    // --- Utility Functions ---
    
    /**
     * Convert float to half-float (16-bit)
     */
    static uint16_t floatToHalf(float value) {
        uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
        uint16_t sign = (bits >> 16) & 0x8000;
        int32_t exponent = ((bits >> 23) & 0xFF) - 127 + 15;
        uint32_t mantissa = bits & 0x7FFFFF;
        
        if (exponent <= 0) {
            return sign; // Zero or denormal
        }
        if (exponent >= 31) {
            return sign | 0x7C00; // Infinity
        }
        
        return sign | (exponent << 10) | (mantissa >> 13);
    }
    
    /**
     * Convert half-float to float
     */
    static float halfToFloat(uint16_t half) {
        uint32_t sign = (half & 0x8000) << 16;
        int32_t exponent = (half >> 10) & 0x1F;
        uint32_t mantissa = half & 0x3FF;
        
        if (exponent == 0) {
            return *reinterpret_cast<float*>(&sign); // Zero
        }
        if (exponent == 31) {
            uint32_t inf = sign | 0x7F800000;
            return *reinterpret_cast<float*>(&inf); // Infinity
        }
        
        exponent = exponent - 15 + 127;
        uint32_t bits = sign | (exponent << 23) | (mantissa << 13);
        return *reinterpret_cast<float*>(&bits);
    }
};

/**
 * Batch compression utilities for converting mesh vertices to light vertices
 */
namespace LightVertexBatch {
    /**
     * Convert array of mesh vertices to compact light vertices
     * @param meshPositions Input mesh positions
     * @param meshNormals Input mesh normals
     * @param meshColors Input mesh colors (radiance)
     * @param vertexCount Number of vertices
     * @param instanceId Instance ID for all vertices
     * @param boundsMin Scene bounds minimum
     * @param boundsMax Scene bounds maximum
     * @param output Output buffer (must be pre-allocated)
     */
    inline void compressVertices(
        const glm::vec3* meshPositions,
        const glm::vec3* meshNormals,
        const glm::vec3* meshColors,
        uint32_t vertexCount,
        uint16_t instanceId,
        const glm::vec3& boundsMin,
        const glm::vec3& boundsMax,
        LightVertexCompact* output
    ) {
        for (uint32_t i = 0; i < vertexCount; ++i) {
            LightVertexCompact& lv = output[i];
            
            LightVertexCompact::encodePosition(lv, meshPositions[i], boundsMin, boundsMax);
            LightVertexCompact::encodeRGBE(lv, meshColors[i]);
            LightVertexCompact::encodeNormal(lv, meshNormals[i]);
            LightVertexCompact::encodeRadiusPdf(lv, 1.0f, 1.0f); // Default values
            
            lv.instanceId = instanceId;
            lv.flags = 0; // Default: diffuse material, LOD 0
            lv.padding = 0;
        }
    }
    
    /**
     * Decompress light vertices back to mesh format
     */
    inline void decompressVertices(
        const LightVertexCompact* input,
        uint32_t vertexCount,
        const glm::vec3& boundsMin,
        const glm::vec3& boundsMax,
        glm::vec3* outPositions,
        glm::vec3* outNormals,
        glm::vec3* outColors
    ) {
        for (uint32_t i = 0; i < vertexCount; ++i) {
            const LightVertexCompact& lv = input[i];
            
            outPositions[i] = LightVertexCompact::decodePosition(lv, boundsMin, boundsMax);
            outNormals[i] = LightVertexCompact::decodeNormal(lv);
            outColors[i] = LightVertexCompact::decodeRGBE(lv);
        }
    }
}

// Ensure struct is exactly 24 bytes
static_assert(sizeof(LightVertexCompact) == 24, "LightVertexCompact must be 24 bytes");
