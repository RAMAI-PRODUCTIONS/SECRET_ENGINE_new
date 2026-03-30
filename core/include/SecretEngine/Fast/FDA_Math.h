// ============================================================================
// SECRETENGINE - FDA COMPLIANT MATH (8-BYTE PACKETS)
// All structures fit in 8 bytes for ultra-fast lock-free transmission
// ============================================================================

#pragma once
#include <cstdint>
#include <cmath>

namespace SecretEngine::Fast {

// ============================================================================
// 8-BYTE PACKED VECTOR TYPES (Fixed-Point Encoding)
// ============================================================================

// Encoding scheme:
// - Float range [-1024, 1024] → int16 [-32768, 32767]
// - Precision: 1024 / 32768 = 0.03125 units
// - Perfect for positions, normals, UVs

constexpr float FIXED16_SCALE = 32.0f;  // Precision = 1/32 = 0.03125

inline int16_t FloatToFixed16(float value) {
    return static_cast<int16_t>(value * FIXED16_SCALE);
}

inline float Fixed16ToFloat(int16_t value) {
    return static_cast<float>(value) / FIXED16_SCALE;
}

// ============================================================================
// PackedVector2 (4 bytes) - Fits in half a packet
// ============================================================================
struct PackedVector2 {
    int16_t x, y;
    
    PackedVector2() : x(0), y(0) {}
    PackedVector2(float _x, float _y) {
        x = FloatToFixed16(_x);
        y = FloatToFixed16(_y);
    }
    
    void Set(float _x, float _y) {
        x = FloatToFixed16(_x);
        y = FloatToFixed16(_y);
    }
    
    void Get(float& _x, float& _y) const {
        _x = Fixed16ToFloat(x);
        _y = Fixed16ToFloat(y);
    }
};
static_assert(sizeof(PackedVector2) == 4, "Must be 4 bytes");

// ============================================================================
// PackedVector3 (8 bytes) - Fits EXACTLY in one packet
// ============================================================================
struct PackedVector3 {
    int16_t x, y, z;
    int16_t _pad;  // Padding to 8 bytes
    
    PackedVector3() : x(0), y(0), z(0), _pad(0) {}
    PackedVector3(float _x, float _y, float _z) : _pad(0) {
        x = FloatToFixed16(_x);
        y = FloatToFixed16(_y);
        z = FloatToFixed16(_z);
    }
    
    void Set(float _x, float _y, float _z) {
        x = FloatToFixed16(_x);
        y = FloatToFixed16(_y);
        z = FloatToFixed16(_z);
    }
    
    void Get(float& _x, float& _y, float& _z) const {
        _x = Fixed16ToFloat(x);
        _y = Fixed16ToFloat(y);
        _z = Fixed16ToFloat(z);
    }
};
static_assert(sizeof(PackedVector3) == 8, "Must be 8 bytes");

// ============================================================================
// PackedVector4 (8 bytes) - RGBA color or quaternion
// ============================================================================
struct PackedVector4 {
    int16_t x, y, z, w;
    
    PackedVector4() : x(0), y(0), z(0), w(0) {}
    PackedVector4(float _x, float _y, float _z, float _w) {
        x = FloatToFixed16(_x);
        y = FloatToFixed16(_y);
        z = FloatToFixed16(_z);
        w = FloatToFixed16(_w);
    }
    
    void Set(float _x, float _y, float _z, float _w) {
        x = FloatToFixed16(_x);
        y = FloatToFixed16(_y);
        z = FloatToFixed16(_z);
        w = FloatToFixed16(_w);
    }
    
    void Get(float& _x, float& _y, float& _z, float& _w) const {
        _x = Fixed16ToFloat(x);
        _y = Fixed16ToFloat(y);
        _z = Fixed16ToFloat(z);
        _w = Fixed16ToFloat(w);
    }
};
static_assert(sizeof(PackedVector4) == 8, "Must be 8 bytes");

// ============================================================================
// PackedColor (8 bytes) - RGBA + intensity + flags
// ============================================================================
struct PackedColor {
    uint16_t r, g, b, a;  // Range [0, 65535] for HDR colors
    
    PackedColor() : r(0), g(0), b(0), a(65535) {}
    PackedColor(float _r, float _g, float _b, float _a = 1.0f) {
        r = static_cast<uint16_t>(_r * 65535.0f);
        g = static_cast<uint16_t>(_g * 65535.0f);
        b = static_cast<uint16_t>(_b * 65535.0f);
        a = static_cast<uint16_t>(_a * 65535.0f);
    }
    
    void Set(float _r, float _g, float _b, float _a = 1.0f) {
        r = static_cast<uint16_t>(_r * 65535.0f);
        g = static_cast<uint16_t>(_g * 65535.0f);
        b = static_cast<uint16_t>(_b * 65535.0f);
        a = static_cast<uint16_t>(_a * 65535.0f);
    }
    
    void Get(float& _r, float& _g, float& _b, float& _a) const {
        _r = r / 65535.0f;
        _g = g / 65535.0f;
        _b = b / 65535.0f;
        _a = a / 65535.0f;
    }
};
static_assert(sizeof(PackedColor) == 8, "Must be 8 bytes");

// ============================================================================
// PackedRotation (8 bytes) - Compressed quaternion
// ============================================================================
// Smallest-3 quaternion compression:
// - Store 3 components as int16
// - Reconstruct 4th component using unit quaternion property
// - 1 byte for which component to reconstruct
// - Total: 6 bytes + 1 byte + 1 padding = 8 bytes

struct PackedRotation {
    int16_t a, b, c;  // 3 largest components
    uint8_t index;    // Which component to reconstruct (0-3)
    uint8_t _pad;
    
    PackedRotation() : a(0), b(0), c(0), index(3), _pad(0) {}
    
    // Pack from quaternion (x, y, z, w)
    void Pack(float x, float y, float z, float w) {
        // Find smallest component
        float components[4] = {x, y, z, w};
        float minVal = fabsf(x);
        index = 0;
        
        for (uint8_t i = 1; i < 4; ++i) {
            if (fabsf(components[i]) < minVal) {
                minVal = fabsf(components[i]);
                index = i;
            }
        }
        
        // Pack the other 3 components
        uint8_t idx = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            if (i != index) {
                int16_t* dest = (idx == 0) ? &a : (idx == 1) ? &b : &c;
                *dest = static_cast<int16_t>(components[i] * 32767.0f);
                idx++;
            }
        }
    }
    
    // Unpack to quaternion
    void Unpack(float& x, float& y, float& z, float& w) {
        float components[3] = {
            a / 32767.0f,
            b / 32767.0f,
            c / 32767.0f
        };
        
        // Reconstruct missing component
        float sumSquares = components[0] * components[0] +
                          components[1] * components[1] +
                          components[2] * components[2];
        float reconstructed = sqrtf(1.0f - sumSquares);
        
        // Fill output
        uint8_t idx = 0;
        float* outputs[4] = {&x, &y, &z, &w};
        for (uint8_t i = 0; i < 4; ++i) {
            if (i == index) {
                *outputs[i] = reconstructed;
            } else {
                *outputs[i] = components[idx++];
            }
        }
    }
};
static_assert(sizeof(PackedRotation) == 8, "Must be 8 bytes");

// ============================================================================
// PackedTransform (8 bytes) - Position only (most common use case)
// ============================================================================
struct PackedTransform {
    PackedVector3 position;  // 8 bytes (with padding)
};
static_assert(sizeof(PackedTransform) == 8, "Must be 8 bytes");

// ============================================================================
// CONVERSION HELPERS (Between FDA and GPU formats)
// ============================================================================

// Convert FDA PackedVector3 → GPU Float3
inline void PackedToGPU(const PackedVector3& packed, float* out) {
    packed.Get(out[0], out[1], out[2]);
}

// Convert GPU Float3 → FDA PackedVector3
inline void GPUToPacked(const float* in, PackedVector3& packed) {
    packed.Set(in[0], in[1], in[2]);
}

} // namespace SecretEngine::Fast
