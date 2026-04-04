// SecretEngine
// Module: core
// Responsibility: New component implementations with lifecycle support
// Inspired by: Overload Engine's component system

#pragma once
#include <SecretEngine/IComponent.h>
#include <SecretEngine/Math.h>
#include <string>
#include <cstring>
#include <functional>

namespace SecretEngine {

// Import math types into SecretEngine namespace
using Vec3 = Math::Vec3;
using Mat4 = Math::Mat4;

/**
 * TransformComponentNew - Manages entity position, rotation, and scale
 * Supports hierarchical transforms (world vs local)
 */
class TransformComponentNew : public IComponent {
public:
    explicit TransformComponentNew(EntityObject& owner)
        : IComponent(owner)
        , m_localPosition(0.0f, 0.0f, 0.0f)
        , m_localRotation(0.0f, 0.0f, 0.0f)
        , m_localScale(1.0f, 1.0f, 1.0f)
        , m_worldMatrixDirty(true)
    {}
    
    COMPONENT_TYPE(TransformComponentNew)
    
    // Local transform
    void SetLocalPosition(const Vec3& position) {
        m_localPosition = position;
        m_worldMatrixDirty = true;
    }
    
    void SetLocalRotation(const Vec3& rotation) {
        m_localRotation = rotation;
        m_worldMatrixDirty = true;
    }
    
    void SetLocalScale(const Vec3& scale) {
        m_localScale = scale;
        m_worldMatrixDirty = true;
    }
    
    const Vec3& GetLocalPosition() const { return m_localPosition; }
    const Vec3& GetLocalRotation() const { return m_localRotation; }
    const Vec3& GetLocalScale() const { return m_localScale; }
    
    // World transform (considers parent hierarchy)
    Vec3 GetWorldPosition() const;
    Vec3 GetWorldRotation() const;
    Vec3 GetWorldScale() const;
    
    // Matrix generation
    Mat4 GetLocalMatrix() const;
    Mat4 GetWorldMatrix() const;
    
    // Direction vectors
    Vec3 GetForward() const;
    Vec3 GetUp() const;
    Vec3 GetRight() const;
    
    // Serialization
    void Serialize(void* outData) const override;
    void Deserialize(const void* inData) override;
    
private:
    Vec3 m_localPosition;
    Vec3 m_localRotation; // Euler angles in degrees
    Vec3 m_localScale;
    
    mutable bool m_worldMatrixDirty;
    mutable Mat4 m_cachedWorldMatrix;
};

/**
 * MeshComponentNew - Renders a 3D mesh with material properties
 */
class MeshComponentNew : public IComponent {
public:
    explicit MeshComponentNew(EntityObject& owner)
        : IComponent(owner)
        , m_visible(true)
    {
        m_meshPath[0] = '\0';
        m_texturePath[0] = '\0';
        m_normalMapPath[0] = '\0';
        m_color[0] = 1.0f; m_color[1] = 1.0f; m_color[2] = 1.0f; m_color[3] = 1.0f;
    }
    
    COMPONENT_TYPE(MeshComponentNew)
    
    void SetMeshPath(const char* path) {
        strncpy(m_meshPath, path, sizeof(m_meshPath) - 1);
        m_meshPath[sizeof(m_meshPath) - 1] = '\0';
    }
    
    void SetTexturePath(const char* path) {
        strncpy(m_texturePath, path, sizeof(m_texturePath) - 1);
        m_texturePath[sizeof(m_texturePath) - 1] = '\0';
    }
    
    void SetNormalMapPath(const char* path) {
        strncpy(m_normalMapPath, path, sizeof(m_normalMapPath) - 1);
        m_normalMapPath[sizeof(m_normalMapPath) - 1] = '\0';
    }
    
    void SetColor(float r, float g, float b, float a = 1.0f) {
        m_color[0] = r; m_color[1] = g; m_color[2] = b; m_color[3] = a;
    }
    
    void SetVisible(bool visible) { m_visible = visible; }
    
    const char* GetMeshPath() const { return m_meshPath; }
    const char* GetTexturePath() const { return m_texturePath; }
    const char* GetNormalMapPath() const { return m_normalMapPath; }
    const float* GetColor() const { return m_color; }
    bool IsVisible() const { return m_visible; }
    
    // Serialization
    void Serialize(void* outData) const override;
    void Deserialize(const void* inData) override;
    
private:
    char m_meshPath[256];
    char m_texturePath[256];
    char m_normalMapPath[256];
    float m_color[4]; // RGBA
    bool m_visible;
};

/**
 * CameraComponentNew - Defines a camera for rendering
 */
class CameraComponentNew : public IComponent {
public:
    explicit CameraComponentNew(EntityObject& owner)
        : IComponent(owner)
        , m_fov(60.0f)
        , m_nearPlane(0.1f)
        , m_farPlane(1000.0f)
        , m_isMainCamera(false)
    {}
    
    COMPONENT_TYPE(CameraComponentNew)
    
    void SetFOV(float fov) { m_fov = fov; }
    void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
    void SetFarPlane(float farPlane) { m_farPlane = farPlane; }
    void SetMainCamera(bool isMain) { m_isMainCamera = isMain; }
    
    float GetFOV() const { return m_fov; }
    float GetNearPlane() const { return m_nearPlane; }
    float GetFarPlane() const { return m_farPlane; }
    bool IsMainCamera() const { return m_isMainCamera; }
    
    Mat4 GetProjectionMatrix(float aspectRatio) const;
    Mat4 GetViewMatrix() const;
    
    // Serialization
    void Serialize(void* outData) const override;
    void Deserialize(const void* inData) override;
    
private:
    float m_fov;
    float m_nearPlane;
    float m_farPlane;
    bool m_isMainCamera;
};

/**
 * LightComponentNew - Defines a light source
 */
class LightComponentNew : public IComponent {
public:
    enum class LightType {
        Directional,
        Point,
        Spot
    };
    
    explicit LightComponentNew(EntityObject& owner)
        : IComponent(owner)
        , m_type(LightType::Point)
        , m_intensity(1.0f)
        , m_range(10.0f)
        , m_spotAngle(45.0f)
    {
        m_color[0] = 1.0f; m_color[1] = 1.0f; m_color[2] = 1.0f;
    }
    
    COMPONENT_TYPE(LightComponentNew)
    
    void SetType(LightType type) { m_type = type; }
    void SetColor(float r, float g, float b) {
        m_color[0] = r; m_color[1] = g; m_color[2] = b;
    }
    void SetIntensity(float intensity) { m_intensity = intensity; }
    void SetRange(float range) { m_range = range; }
    void SetSpotAngle(float angle) { m_spotAngle = angle; }
    
    LightType GetType() const { return m_type; }
    const float* GetColor() const { return m_color; }
    float GetIntensity() const { return m_intensity; }
    float GetRange() const { return m_range; }
    float GetSpotAngle() const { return m_spotAngle; }
    
    // Serialization
    void Serialize(void* outData) const override;
    void Deserialize(const void* inData) override;
    
private:
    LightType m_type;
    float m_color[3]; // RGB
    float m_intensity;
    float m_range;
    float m_spotAngle;
};

/**
 * ScriptComponentNew - Allows custom behavior through callbacks
 */
class ScriptComponentNew : public IComponent {
public:
    using UpdateCallback = std::function<void(float)>;
    
    explicit ScriptComponentNew(EntityObject& owner)
        : IComponent(owner)
    {}
    
    COMPONENT_TYPE(ScriptComponentNew)
    
    void SetUpdateCallback(UpdateCallback callback) {
        m_updateCallback = callback;
    }
    
    void OnUpdate(float deltaTime) override {
        if (m_updateCallback) {
            m_updateCallback(deltaTime);
        }
    }
    
private:
    UpdateCallback m_updateCallback;
};

} // namespace SecretEngine
