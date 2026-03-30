// SecretEngine
// Module: PhysicsPlugin
// Responsibility: Physics type definitions
// Dependencies: Core

#pragma once
#include <cstdint>

namespace SecretEngine::Physics {

// Collision shapes
enum class ShapeType : uint8_t {
    Sphere,
    Box,
    Capsule,
    None
};

// Physics body types
enum class BodyType : uint8_t {
    Static,      // Immovable (walls, floors)
    Dynamic,     // Fully simulated (rigidbodies)
    Kinematic    // Movable but not affected by forces (character controllers)
};

// Collision layers (bitfield)
enum CollisionLayer : uint32_t {
    Default = 1 << 0,
    Player = 1 << 1,
    Enemy = 1 << 2,
    Projectile = 1 << 3,
    Environment = 1 << 4,
    Trigger = 1 << 5,
    All = 0xFFFFFFFF
};

// Shape definitions
struct SphereShape {
    float radius;
};

struct BoxShape {
    float halfExtents[3]; // Half-width, half-height, half-depth
};

struct CapsuleShape {
    float radius;
    float halfHeight; // Half of cylinder height (not including caps)
};

// Physics material
struct PhysicsMaterial {
    float friction = 0.5f;
    float restitution = 0.0f; // Bounciness (0 = no bounce, 1 = perfect bounce)
    float density = 1.0f;
};

// Collision info
struct CollisionInfo {
    uint32_t entityA;
    uint32_t entityB;
    float point[3];
    float normal[3];
    float penetration;
    float impulse;
};

// Raycast result
struct RaycastHit {
    bool hit = false;
    uint32_t entityId = 0;
    float point[3] = {0, 0, 0};
    float normal[3] = {0, 1, 0};
    float distance = 0.0f;
};

// Physics body data (POD component)
struct PhysicsBody {
    static constexpr uint32_t TypeID = 0x2001;
    
    BodyType bodyType = BodyType::Dynamic;
    ShapeType shapeType = ShapeType::Sphere;
    
    // Shape data (union would be better but keeping it simple)
    float shapeData[4] = {0.5f, 0, 0, 0}; // sphere radius / box extents / capsule radius+height
    
    // Physics properties
    float mass = 1.0f;
    float inverseMass = 1.0f;
    
    // Linear motion
    float velocity[3] = {0, 0, 0};
    float acceleration[3] = {0, 0, 0};
    
    // Angular motion (simplified - just Y-axis rotation for now)
    float angularVelocity = 0.0f;
    
    // Forces
    float force[3] = {0, 0, 0};
    float torque = 0.0f;
    
    // Material
    PhysicsMaterial material;
    
    // Collision filtering
    uint32_t collisionLayer = CollisionLayer::Default;
    uint32_t collisionMask = CollisionLayer::All;
    
    // Flags
    bool useGravity = true;
    bool isTrigger = false;
    bool isGrounded = false;
    bool lockRotation = false;
    
    // Constraints
    bool freezePositionX = false;
    bool freezePositionY = false;
    bool freezePositionZ = false;
};

} // namespace SecretEngine::Physics
