// SecretEngine
// Module: core
// Responsibility: Component implementations

#include <SecretEngine/ComponentsNew.h>
#include <SecretEngine/EntityObject.h>
#include <SecretEngine/Components.h> // For old POD structures
#include <cmath>
#include <cstring>

namespace SecretEngine {

// Helper functions for Vec3 operations
static inline Vec3 Vec3Add(const Vec3& a, const Vec3& b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vec3 Vec3Multiply(const Vec3& a, const Vec3& b) {
    return Vec3{a.x * b.x, a.y * b.y, a.z * b.z};
}

static inline float Vec3Length(const Vec3& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vec3 Vec3Normalize(const Vec3& v) {
    float len = Vec3Length(v);
    if (len > 0.0001f) {
        return Vec3{v.x / len, v.y / len, v.z / len};
    }
    return Vec3{0.0f, 0.0f, 1.0f};
}

// ============================================================================
// TransformComponentNew Implementation
// ============================================================================

Vec3 TransformComponentNew::GetWorldPosition() const {
    EntityObject* parent = m_owner.GetParent();
    if (!parent) {
        return m_localPosition;
    }
    
    TransformComponentNew* parentTransform = parent->GetComponent<TransformComponentNew>();
    if (!parentTransform) {
        return m_localPosition;
    }
    
    // Simple world position calculation
    Vec3 parentPos = parentTransform->GetWorldPosition();
    return Vec3Add(parentPos, m_localPosition);
}

Vec3 TransformComponentNew::GetWorldRotation() const {
    EntityObject* parent = m_owner.GetParent();
    if (!parent) {
        return m_localRotation;
    }
    
    TransformComponentNew* parentTransform = parent->GetComponent<TransformComponentNew>();
    if (!parentTransform) {
        return m_localRotation;
    }
    
    // Simple world rotation calculation
    Vec3 parentRot = parentTransform->GetWorldRotation();
    return Vec3Add(parentRot, m_localRotation);
}

Vec3 TransformComponentNew::GetWorldScale() const {
    EntityObject* parent = m_owner.GetParent();
    if (!parent) {
        return m_localScale;
    }
    
    TransformComponentNew* parentTransform = parent->GetComponent<TransformComponentNew>();
    if (!parentTransform) {
        return m_localScale;
    }
    
    // World scale is multiplied
    Vec3 parentScale = parentTransform->GetWorldScale();
    return Vec3Multiply(parentScale, m_localScale);
}

Mat4 TransformComponentNew::GetLocalMatrix() const {
    // Use Math::Matrix4x4 static methods
    float uniformScale = (m_localScale.x + m_localScale.y + m_localScale.z) / 3.0f;
    
    // For now, use simplified transform (can be enhanced later)
    Math::Matrix3x4 transform;
    Math::Matrix4x4::FromTRS(
        transform,
        m_localPosition.x, m_localPosition.y, m_localPosition.z,
        m_localRotation.x * 3.14159f / 180.0f, // Convert to radians
        m_localRotation.y * 3.14159f / 180.0f,
        m_localRotation.z * 3.14159f / 180.0f,
        uniformScale
    );
    
    // Convert Matrix3x4 to Matrix4x4
    Mat4 result = Mat4::Identity();
    for (int i = 0; i < 12; i++) {
        result.m[i] = transform.m[i];
    }
    return result;
}

Mat4 TransformComponentNew::GetWorldMatrix() const {
    if (m_worldMatrixDirty) {
        EntityObject* parent = m_owner.GetParent();
        if (parent) {
            TransformComponentNew* parentTransform = parent->GetComponent<TransformComponentNew>();
            if (parentTransform) {
                m_cachedWorldMatrix = parentTransform->GetWorldMatrix() * GetLocalMatrix();
            } else {
                m_cachedWorldMatrix = GetLocalMatrix();
            }
        } else {
            m_cachedWorldMatrix = GetLocalMatrix();
        }
        m_worldMatrixDirty = false;
    }
    
    return m_cachedWorldMatrix;
}

Vec3 TransformComponentNew::GetForward() const {
    // In Z-up coordinate system, Y is forward
    // Extract from rotation matrix (simplified)
    float ry = m_localRotation.y * 3.14159f / 180.0f;
    return Vec3Normalize(Vec3{sinf(ry), cosf(ry), 0.0f});
}

Vec3 TransformComponentNew::GetUp() const {
    // In Z-up coordinate system, Z is up
    return Vec3{0.0f, 0.0f, 1.0f};
}

Vec3 TransformComponentNew::GetRight() const {
    // In Z-up coordinate system, X is right
    // Extract from rotation matrix (simplified)
    float ry = m_localRotation.y * 3.14159f / 180.0f;
    return Vec3Normalize(Vec3{cosf(ry), -sinf(ry), 0.0f});
}

void TransformComponentNew::Serialize(void* outData) const {
    if (!outData) return;
    
    TransformComponent* data = static_cast<TransformComponent*>(outData);
    data->position[0] = m_localPosition.x;
    data->position[1] = m_localPosition.y;
    data->position[2] = m_localPosition.z;
    data->rotation[0] = m_localRotation.x;
    data->rotation[1] = m_localRotation.y;
    data->rotation[2] = m_localRotation.z;
    data->scale[0] = m_localScale.x;
    data->scale[1] = m_localScale.y;
    data->scale[2] = m_localScale.z;
}

void TransformComponentNew::Deserialize(const void* inData) {
    if (!inData) return;
    
    const TransformComponent* data = static_cast<const TransformComponent*>(inData);
    m_localPosition = Vec3{data->position[0], data->position[1], data->position[2]};
    m_localRotation = Vec3{data->rotation[0], data->rotation[1], data->rotation[2]};
    m_localScale = Vec3{data->scale[0], data->scale[1], data->scale[2]};
    m_worldMatrixDirty = true;
}

// ============================================================================
// MeshComponentNew Implementation
// ============================================================================

void MeshComponentNew::Serialize(void* outData) const {
    if (!outData) return;
    
    MeshComponent* data = static_cast<MeshComponent*>(outData);
    strncpy(data->meshPath, m_meshPath, sizeof(data->meshPath) - 1);
    strncpy(data->texturePath, m_texturePath, sizeof(data->texturePath) - 1);
    strncpy(data->normalMapPath, m_normalMapPath, sizeof(data->normalMapPath) - 1);
    data->color[0] = m_color[0];
    data->color[1] = m_color[1];
    data->color[2] = m_color[2];
    data->color[3] = m_color[3];
}

void MeshComponentNew::Deserialize(const void* inData) {
    if (!inData) return;
    
    const MeshComponent* data = static_cast<const MeshComponent*>(inData);
    strncpy(m_meshPath, data->meshPath, sizeof(m_meshPath) - 1);
    strncpy(m_texturePath, data->texturePath, sizeof(m_texturePath) - 1);
    strncpy(m_normalMapPath, data->normalMapPath, sizeof(m_normalMapPath) - 1);
    m_color[0] = data->color[0];
    m_color[1] = data->color[1];
    m_color[2] = data->color[2];
    m_color[3] = data->color[3];
}

// ============================================================================
// CameraComponentNew Implementation
// ============================================================================

Mat4 CameraComponentNew::GetProjectionMatrix(float aspectRatio) const {
    return Mat4::Perspective(m_fov * 3.14159f / 180.0f, aspectRatio, m_nearPlane, m_farPlane);
}

Mat4 CameraComponentNew::GetViewMatrix() const {
    TransformComponentNew* transform = m_owner.GetComponent<TransformComponentNew>();
    if (!transform) {
        return Mat4::Identity();
    }
    
    // Simplified view matrix (can be enhanced with proper LookAt)
    Vec3 position = transform->GetWorldPosition();
    Mat4 view = Mat4::Translation(-position.x, -position.y, -position.z);
    return view;
}

void CameraComponentNew::Serialize(void* outData) const {
    // TODO: Define camera POD structure if needed
}

void CameraComponentNew::Deserialize(const void* inData) {
    // TODO: Implement when camera POD structure is defined
}

// ============================================================================
// LightComponentNew Implementation
// ============================================================================

void LightComponentNew::Serialize(void* outData) const {
    // TODO: Define light POD structure if needed
}

void LightComponentNew::Deserialize(const void* inData) {
    // TODO: Implement when light POD structure is defined
}

} // namespace SecretEngine
