// ============================================================================
// SECRET ENGINE - CORE MATH LIBRARY
// Header-only, high-performance, SIMD-aligned (POD)
// ============================================================================
#pragma once
#include <cmath>
#include <cstring>

namespace SecretEngine::Math {

#define SE_MATH_INLINE [[gnu::always_inline]] inline

struct Float2 {
    float x, y;
};

struct alignas(16) Float3 {
    float x, y, z;
    float padding; // Ensure 16-byte alignment
};

struct alignas(16) Float4 {
    float x, y, z, w;
};

struct alignas(16) Quaternion {
    float x, y, z, w;

    SE_MATH_INLINE static Quaternion Identity() noexcept {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
};

struct alignas(16) Matrix3x4 {
    float m[12]; // Column-major 3x4 (last row 0,0,0,1 implied)
};

struct alignas(16) Matrix4x4 {
    float m[16]; // Column-major

    SE_MATH_INLINE static Matrix4x4 Identity() noexcept {
        return Matrix4x4 {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        }};
    }

    SE_MATH_INLINE Matrix4x4 operator*(const Matrix4x4& other) const noexcept {
        Matrix4x4 res;
        const float* a = m;
        const float* b = other.m;
        float* r = res.m;

        // Unrolled Column-Major Multiplication (High-Performance Path)
        r[0] = a[0]*b[0] + a[4]*b[1] + a[8]*b[2] + a[12]*b[3];
        r[1] = a[1]*b[0] + a[5]*b[1] + a[9]*b[2] + a[13]*b[3];
        r[2] = a[2]*b[0] + a[6]*b[1] + a[10]*b[2] + a[14]*b[3];
        r[3] = a[3]*b[0] + a[7]*b[1] + a[11]*b[2] + a[15]*b[3];

        r[4] = a[0]*b[4] + a[4]*b[5] + a[8]*b[6] + a[12]*b[7];
        r[5] = a[1]*b[4] + a[5]*b[5] + a[9]*b[6] + a[13]*b[7];
        r[6] = a[2]*b[4] + a[6]*b[5] + a[10]*b[6] + a[14]*b[7];
        r[7] = a[3]*b[4] + a[7]*b[5] + a[11]*b[6] + a[15]*b[7];

        r[8] = a[0]*b[8] + a[4]*b[9] + a[8]*b[10] + a[12]*b[11];
        r[9] = a[1]*b[8] + a[5]*b[9] + a[9]*b[10] + a[13]*b[11];
        r[10] = a[2]*b[8] + a[6]*b[9] + a[10]*b[10] + a[14]*b[11];
        r[11] = a[3]*b[8] + a[7]*b[9] + a[11]*b[10] + a[15]*b[11];

        r[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];
        r[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];
        r[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
        r[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];

        return res;
    }

    SE_MATH_INLINE static Matrix4x4 Translation(float x, float y, float z) noexcept {
        Matrix4x4 res = Identity();
        res.m[12] = x; res.m[13] = y; res.m[14] = z;
        return res;
    }

    SE_MATH_INLINE static Matrix4x4 RotationX(float a) noexcept {
        Matrix4x4 res = Identity();
        float c = cosf(a), s = sinf(a);
        res.m[5] = c; res.m[6] = s; res.m[9] = -s; res.m[10] = c;
        return res;
    }

    SE_MATH_INLINE static Matrix4x4 RotationY(float a) noexcept {
        Matrix4x4 res = Identity();
        float c = cosf(a), s = sinf(a);
        res.m[0] = c; res.m[2] = -s; res.m[8] = s; res.m[10] = c;
        return res;
    }

    SE_MATH_INLINE static Matrix4x4 RotationZ(float a) noexcept {
        Matrix4x4 res = Identity();
        float c = cosf(a), s = sinf(a);
        res.m[0] = c; res.m[1] = s; res.m[4] = -s; res.m[5] = c;
        return res;
    }

    SE_MATH_INLINE static Matrix4x4 Scale(float x, float y, float z) noexcept {
        Matrix4x4 res = Identity();
        res.m[0] = x; res.m[5] = y; res.m[10] = z;
        return res;
    }

    // ULTRA-FAST TRS: Optimized for common case (Y-axis rotation only)
    // Eliminates 12 trig calls and 30+ multiplications per instance
    SE_MATH_INLINE static void FromTRS(Matrix3x4& out, float tx, float ty, float tz, float rx, float ry, float rz, float s) noexcept {
        // Fast path: Y-rotation only (most common in games)
        if (rx == 0.0f && rz == 0.0f) {
            float cy = cosf(ry), sy = sinf(ry);
            float ss = s; // Pre-scaled
            
            // Row 0: [cy*s, 0, sy*s, tx]
            out.m[0] = cy * ss;
            out.m[1] = 0.0f;
            out.m[2] = sy * ss;
            out.m[3] = tx;
            
            // Row 1: [0, s, 0, ty]
            out.m[4] = 0.0f;
            out.m[5] = ss;
            out.m[6] = 0.0f;
            out.m[7] = ty;
            
            // Row 2: [-sy*s, 0, cy*s, tz]
            out.m[8] = -sy * ss;
            out.m[9] = 0.0f;
            out.m[10] = cy * ss;
            out.m[11] = tz;
            return;
        }
        
        // Full path: All rotations (fallback)
        float cx = cosf(rx), sx = sinf(rx);
        float cy = cosf(ry), sy = sinf(ry);
        float cz = cosf(rz), sz = sinf(rz);
        
        // Pre-compute common terms
        float sxsy = sx * sy;
        float cxsy = cx * sy;
        float cycz = cy * cz;
        float cysz = cy * sz;

        // Row 0
        out.m[0] = cycz * s;
        out.m[1] = -cysz * s;
        out.m[2] = sy * s;
        out.m[3] = tx;

        // Row 1
        out.m[4] = (sxsy * cz + cx * sz) * s;
        out.m[5] = (-sxsy * sz + cx * cz) * s;
        out.m[6] = -sx * cy * s;
        out.m[7] = ty;

        // Row 2
        out.m[8] = (-cxsy * cz + sx * sz) * s;
        out.m[9] = (cxsy * sz + sx * cz) * s;
        out.m[10] = cx * cy * s;
        out.m[11] = tz;
    }
    
    SE_MATH_INLINE static Matrix4x4 Perspective(float fov, float aspect, float nearP, float farP) noexcept {
        float f = 1.0f / tanf(fov / 2.0f);
        Matrix4x4 res = {};
        res.m[0] = f / aspect;
        res.m[5] = -f; // Vulkan Y is inverted
        res.m[10] = farP / (nearP - farP);
        res.m[11] = -1.0f;
        res.m[14] = (nearP * farP) / (nearP - farP);
        return res;
    }

    // Fast Frustum Plane Extraction (Gribb-Hartmann method)
    // Used for ultra-fast visibility testing in MegaGeometryRenderer
    SE_MATH_INLINE static void ExtractFrustumPlanes(const Matrix4x4& vp, float planes[6][4]) noexcept {
        const float* m = vp.m;
        // Left Plane
        planes[0][0]=m[3]+m[0]; planes[0][1]=m[7]+m[4]; planes[0][2]=m[11]+m[8]; planes[0][3]=m[15]+m[12];
        // Right Plane
        planes[1][0]=m[3]-m[0]; planes[1][1]=m[7]-m[4]; planes[1][2]=m[11]-m[8]; planes[1][3]=m[15]-m[12];
        // Bottom Plane
        planes[2][0]=m[3]+m[1]; planes[2][1]=m[7]+m[5]; planes[2][2]=m[11]+m[9]; planes[2][3]=m[15]+m[13];
        // Top Plane
        planes[3][0]=m[3]-m[1]; planes[3][1]=m[7]-m[5]; planes[3][2]=m[11]-m[9]; planes[3][3]=m[15]-m[13];
        // Near Plane
        planes[4][0]=m[3]+m[2]; planes[4][1]=m[7]+m[6]; planes[4][2]=m[11]+m[10];planes[4][3]=m[15]+m[14];
        // Far Plane
        planes[5][0]=m[3]-m[2]; planes[5][1]=m[7]-m[6]; planes[5][2]=m[11]-m[10];planes[5][3]=m[15]-m[14];

        // Normalize planes for accurate distance testing
        for(int i=0; i<6; ++i) {
            float length = sqrtf(planes[i][0]*planes[i][0] + planes[i][1]*planes[i][1] + planes[i][2]*planes[i][2]);
            planes[i][0]/=length; planes[i][1]/=length; planes[i][2]/=length; planes[i][3]/=length;
        }
    }
};

struct alignas(32) InstanceData {
    Matrix3x4 transform;   // 48 bytes
    uint32_t  packedColor; // 4 bytes (8-bit RGBA)
    uint32_t  textureID;   // 4 bytes (bindless texture index, UINT32_MAX = no texture)
    uint32_t  _padding[2]; // Align to 64 bytes
};
static_assert(sizeof(InstanceData) == 64, "InstanceData must be 64 bytes for cache line alignment");

} // namespace SecretEngine::Math
