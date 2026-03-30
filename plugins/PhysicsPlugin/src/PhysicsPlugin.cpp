// SecretEngine
// Module: PhysicsPlugin
// Responsibility: Comprehensive physics implementation
// Dependencies: Core, PhysicsTypes, CollisionDetection

#include "PhysicsPlugin.h"
#include "CollisionDetection.h"
#include <algorithm>
#include <cmath>

namespace SecretEngine::Physics {

using namespace Math;

void PhysicsPlugin::OnLoad(ICore* core) {
    m_core = core;
    m_world = core->GetWorld();
    m_logger = core->GetLogger();
    
    m_logger->LogInfo("PhysicsPlugin", "loaded - Full physics system initialized");
    
    // Register capability
    core->RegisterCapability("physics", this);
}

void PhysicsPlugin::OnActivate() {
    m_logger->LogInfo("PhysicsPlugin", "activated");
}

void PhysicsPlugin::OnDeactivate() {
    m_logger->LogInfo("PhysicsPlugin", "deactivated");
}

void PhysicsPlugin::OnUnload() {
    m_logger->LogInfo("PhysicsPlugin", "unloaded");
}

void PhysicsPlugin::OnUpdate(float dt) {
    // Fixed timestep physics
    m_accumulator += dt;
    
    int steps = 0;
    while (m_accumulator >= m_fixedTimeStep && steps < m_maxSubSteps) {
        FixedUpdate(m_fixedTimeStep);
        m_accumulator -= m_fixedTimeStep;
        steps++;
    }
    
    // Clamp accumulator to prevent spiral of death
    if (m_accumulator > m_fixedTimeStep * m_maxSubSteps) {
        m_accumulator = 0.0f;
    }
}

void PhysicsPlugin::FixedUpdate(float dt) {
    // Physics pipeline
    IntegrateVelocity(dt);
    DetectCollisions();
    ResolveCollisions();
    IntegratePosition(dt);
}

void PhysicsPlugin::IntegrateVelocity(float dt) {
    auto& entities = m_world->GetAllEntities();
    
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (!body || body->bodyType != BodyType::Dynamic) continue;
        
        // Apply gravity
        if (body->useGravity) {
            ScaleAdd(body->acceleration, m_gravity, 1.0f, body->acceleration);
        }
        
        // Apply forces: F = ma, a = F/m
        if (body->inverseMass > 0.0f) {
            float forceAccel[3];
            Scale(body->force, body->inverseMass, forceAccel);
            Add(body->acceleration, forceAccel, body->acceleration);
        }
        
        // Integrate velocity: v = v + a*dt
        ScaleAdd(body->velocity, body->acceleration, dt, body->velocity);
        
        // Apply damping (air resistance)
        float damping = 0.99f;
        Scale(body->velocity, damping, body->velocity);
        
        // Clear forces and acceleration
        body->force[0] = body->force[1] = body->force[2] = 0;
        body->acceleration[0] = body->acceleration[1] = body->acceleration[2] = 0;
    }
}

void PhysicsPlugin::DetectCollisions() {
    m_collisions.clear();
    
    auto& entities = m_world->GetAllEntities();
    std::vector<Entity> physicsEntities;
    
    // Collect entities with physics bodies
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (body) {
            physicsEntities.push_back(entity);
        }
    }
    
    // Broad phase: Check all pairs
    for (size_t i = 0; i < physicsEntities.size(); i++) {
        for (size_t j = i + 1; j < physicsEntities.size(); j++) {
            Entity entityA = physicsEntities[i];
            Entity entityB = physicsEntities[j];
            
            auto* bodyA = static_cast<PhysicsBody*>(
                m_world->GetComponent(entityA, PhysicsBody::TypeID)
            );
            auto* bodyB = static_cast<PhysicsBody*>(
                m_world->GetComponent(entityB, PhysicsBody::TypeID)
            );
            
            // Skip if both are static
            if (bodyA->bodyType == BodyType::Static && bodyB->bodyType == BodyType::Static) {
                continue;
            }
            
            // Check collision filtering
            if (!ShouldCollide(bodyA->collisionLayer, bodyA->collisionMask,
                              bodyB->collisionLayer, bodyB->collisionMask)) {
                continue;
            }
            
            auto* transformA = static_cast<TransformComponent*>(
                m_world->GetComponent(entityA, TransformComponent::TypeID)
            );
            auto* transformB = static_cast<TransformComponent*>(
                m_world->GetComponent(entityB, TransformComponent::TypeID)
            );
            
            if (!transformA || !transformB) continue;
            
            // Narrow phase: Detailed collision test
            CollisionInfo collision;
            if (TestCollision(transformA, bodyA, transformB, bodyB, &collision)) {
                collision.entityA = entityA.id;
                collision.entityB = entityB.id;
                
                // Skip triggers (they don't resolve)
                if (!bodyA->isTrigger && !bodyB->isTrigger) {
                    m_collisions.push_back(collision);
                }
            }
        }
    }
}

void PhysicsPlugin::ResolveCollisions() {
    for (const CollisionInfo& collision : m_collisions) {
        Entity entityA = {collision.entityA, 1};
        Entity entityB = {collision.entityB, 1};
        
        auto* bodyA = static_cast<PhysicsBody*>(
            m_world->GetComponent(entityA, PhysicsBody::TypeID)
        );
        auto* bodyB = static_cast<PhysicsBody*>(
            m_world->GetComponent(entityB, PhysicsBody::TypeID)
        );
        
        auto* transformA = static_cast<TransformComponent*>(
            m_world->GetComponent(entityA, TransformComponent::TypeID)
        );
        auto* transformB = static_cast<TransformComponent*>(
            m_world->GetComponent(entityB, TransformComponent::TypeID)
        );
        
        if (bodyA && bodyB && transformA && transformB) {
            ResolveCollision(bodyA, transformA, bodyB, transformB, collision);
        }
    }
}

void PhysicsPlugin::IntegratePosition(float dt) {
    auto& entities = m_world->GetAllEntities();
    
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (!body || body->bodyType == BodyType::Static) continue;
        
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(entity, TransformComponent::TypeID)
        );
        if (!transform) continue;
        
        // Integrate position: p = p + v*dt
        if (!body->freezePositionX) transform->position[0] += body->velocity[0] * dt;
        if (!body->freezePositionY) transform->position[1] += body->velocity[1] * dt;
        if (!body->freezePositionZ) transform->position[2] += body->velocity[2] * dt;
        
        // Integrate rotation (simplified Y-axis only)
        if (!body->lockRotation) {
            transform->rotation[1] += body->angularVelocity * dt;
        }
    }
}

bool PhysicsPlugin::TestCollision(
    const TransformComponent* transformA, const PhysicsBody* bodyA,
    const TransformComponent* transformB, const PhysicsBody* bodyB,
    CollisionInfo* outInfo
) {
    // Dispatch based on shape types
    if (bodyA->shapeType == ShapeType::Sphere && bodyB->shapeType == ShapeType::Sphere) {
        return CollisionDetection::TestSphereSphere(
            transformA->position, bodyA->shapeData[0],
            transformB->position, bodyB->shapeData[0],
            outInfo
        );
    }
    else if (bodyA->shapeType == ShapeType::Box && bodyB->shapeType == ShapeType::Box) {
        return CollisionDetection::TestBoxBox(
            transformA->position, bodyA->shapeData,
            transformB->position, bodyB->shapeData,
            outInfo
        );
    }
    else if (bodyA->shapeType == ShapeType::Sphere && bodyB->shapeType == ShapeType::Box) {
        return CollisionDetection::TestSphereBox(
            transformA->position, bodyA->shapeData[0],
            transformB->position, bodyB->shapeData,
            outInfo
        );
    }
    else if (bodyA->shapeType == ShapeType::Box && bodyB->shapeType == ShapeType::Sphere) {
        bool result = CollisionDetection::TestSphereBox(
            transformB->position, bodyB->shapeData[0],
            transformA->position, bodyA->shapeData,
            outInfo
        );
        if (result && outInfo) {
            // Flip normal
            outInfo->normal[0] = -outInfo->normal[0];
            outInfo->normal[1] = -outInfo->normal[1];
            outInfo->normal[2] = -outInfo->normal[2];
        }
        return result;
    }
    else if (bodyA->shapeType == ShapeType::Capsule || bodyB->shapeType == ShapeType::Capsule) {
        // Simplified capsule collision
        float radiusA = bodyA->shapeType == ShapeType::Capsule ? bodyA->shapeData[0] : bodyA->shapeData[0];
        float radiusB = bodyB->shapeType == ShapeType::Capsule ? bodyB->shapeData[0] : bodyB->shapeData[0];
        return CollisionDetection::TestSphereSphere(
            transformA->position, radiusA,
            transformB->position, radiusB,
            outInfo
        );
    }
    
    return false;
}

void PhysicsPlugin::ResolveCollision(
    PhysicsBody* bodyA, TransformComponent* transformA,
    PhysicsBody* bodyB, TransformComponent* transformB,
    const CollisionInfo& collision
) {
    // Position correction (separate overlapping objects)
    float correction[3];
    Scale(collision.normal, collision.penetration * 0.5f, correction);
    
    if (bodyA->bodyType == BodyType::Dynamic) {
        Sub(transformA->position, correction, transformA->position);
    }
    if (bodyB->bodyType == BodyType::Dynamic) {
        Add(transformB->position, correction, transformB->position);
    }
    
    // Velocity resolution (impulse-based)
    float relativeVel[3];
    Sub(bodyB->velocity, bodyA->velocity, relativeVel);
    
    float velAlongNormal = Dot(relativeVel, collision.normal);
    
    // Don't resolve if velocities are separating
    if (velAlongNormal > 0) return;
    
    // Calculate restitution (bounciness)
    float restitution = std::min(bodyA->material.restitution, bodyB->material.restitution);
    
    // Calculate impulse scalar
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (bodyA->inverseMass + bodyB->inverseMass);
    
    // Apply impulse
    float impulse[3];
    Scale(collision.normal, j, impulse);
    
    if (bodyA->bodyType == BodyType::Dynamic) {
        float impulseA[3];
        Scale(impulse, -bodyA->inverseMass, impulseA);
        Add(bodyA->velocity, impulseA, bodyA->velocity);
    }
    
    if (bodyB->bodyType == BodyType::Dynamic) {
        float impulseB[3];
        Scale(impulse, bodyB->inverseMass, impulseB);
        Add(bodyB->velocity, impulseB, bodyB->velocity);
    }
    
    // Apply friction
    float tangent[3];
    float tangentVel = Dot(relativeVel, collision.normal);
    ScaleAdd(relativeVel, collision.normal, -tangentVel, tangent);
    
    float tangentLength = Length(tangent);
    if (tangentLength > 0.0001f) {
        Normalize(tangent, tangent);
        
        float friction = std::sqrt(bodyA->material.friction * bodyB->material.friction);
        float frictionImpulse = -Dot(relativeVel, tangent) * friction;
        frictionImpulse /= (bodyA->inverseMass + bodyB->inverseMass);
        
        // Clamp friction
        if (std::abs(frictionImpulse) > std::abs(j)) {
            frictionImpulse = j;
        }
        
        float frictionVec[3];
        Scale(tangent, frictionImpulse, frictionVec);
        
        if (bodyA->bodyType == BodyType::Dynamic) {
            float frictionA[3];
            Scale(frictionVec, -bodyA->inverseMass, frictionA);
            Add(bodyA->velocity, frictionA, bodyA->velocity);
        }
        
        if (bodyB->bodyType == BodyType::Dynamic) {
            float frictionB[3];
            Scale(frictionVec, bodyB->inverseMass, frictionB);
            Add(bodyB->velocity, frictionB, bodyB->velocity);
        }
    }
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

RaycastHit PhysicsPlugin::Raycast(
    const float origin[3],
    const float direction[3],
    float maxDistance
) {
    return RaycastWithMask(origin, direction, maxDistance, CollisionLayer::All);
}

RaycastHit PhysicsPlugin::RaycastWithMask(
    const float origin[3],
    const float direction[3],
    float maxDistance,
    uint32_t layerMask
) {
    RaycastHit closestHit;
    closestHit.distance = maxDistance;
    
    float dir[3];
    Normalize(direction, dir);
    
    auto& entities = m_world->GetAllEntities();
    
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (!body) continue;
        
        // Check layer mask
        if ((body->collisionLayer & layerMask) == 0) continue;
        
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(entity, TransformComponent::TypeID)
        );
        if (!transform) continue;
        
        float distance = 0.0f;
        if (RaycastShape(origin, dir, transform, body, distance)) {
            if (distance < closestHit.distance && distance >= 0) {
                closestHit.hit = true;
                closestHit.entityId = entity.id;
                closestHit.distance = distance;
                
                // Calculate hit point
                ScaleAdd(origin, dir, distance, closestHit.point);
                
                // Calculate normal (simplified - point away from center)
                Sub(closestHit.point, transform->position, closestHit.normal);
                Normalize(closestHit.normal, closestHit.normal);
            }
        }
    }
    
    return closestHit;
}

bool PhysicsPlugin::RaycastShape(
    const float origin[3],
    const float direction[3],
    const TransformComponent* transform,
    const PhysicsBody* body,
    float& outDistance
) {
    switch (body->shapeType) {
        case ShapeType::Sphere:
            return CollisionDetection::RaySphere(
                origin, direction,
                transform->position, body->shapeData[0],
                outDistance
            );
            
        case ShapeType::Box:
            return CollisionDetection::RayBox(
                origin, direction,
                transform->position, body->shapeData,
                outDistance
            );
            
        case ShapeType::Capsule:
            // Simplified: treat as sphere
            return CollisionDetection::RaySphere(
                origin, direction,
                transform->position, body->shapeData[0] + body->shapeData[1],
                outDistance
            );
            
        default:
            return false;
    }
}

bool PhysicsPlugin::CheckGround(
    const float position[3],
    float radius,
    float checkDistance
) {
    float down[3] = {0, -1, 0};
    RaycastHit hit = Raycast(position, down, checkDistance);
    
    if (hit.hit) return true;
    
    // Fallback: simple Y check
    return position[1] <= 0.1f;
}

bool PhysicsPlugin::OverlapSphere(
    const float position[3],
    float radius,
    uint32_t layerMask
) {
    auto& entities = m_world->GetAllEntities();
    
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (!body || (body->collisionLayer & layerMask) == 0) continue;
        
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(entity, TransformComponent::TypeID)
        );
        if (!transform) continue;
        
        if (body->shapeType == ShapeType::Sphere) {
            if (CollisionDetection::TestSphereSphere(
                position, radius,
                transform->position, body->shapeData[0],
                nullptr)) {
                return true;
            }
        }
    }
    
    return false;
}

bool PhysicsPlugin::OverlapBox(
    const float position[3],
    const float halfExtents[3],
    uint32_t layerMask
) {
    auto& entities = m_world->GetAllEntities();
    
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (!body || (body->collisionLayer & layerMask) == 0) continue;
        
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(entity, TransformComponent::TypeID)
        );
        if (!transform) continue;
        
        if (body->shapeType == ShapeType::Box) {
            if (CollisionDetection::TestBoxBox(
                position, halfExtents,
                transform->position, body->shapeData,
                nullptr)) {
                return true;
            }
        }
    }
    
    return false;
}

void PhysicsPlugin::AddForce(Entity entity, const float force[3]) {
    auto* body = static_cast<PhysicsBody*>(
        m_world->GetComponent(entity, PhysicsBody::TypeID)
    );
    if (!body || body->bodyType != BodyType::Dynamic) return;
    
    Add(body->force, force, body->force);
}

void PhysicsPlugin::AddImpulse(Entity entity, const float impulse[3]) {
    auto* body = static_cast<PhysicsBody*>(
        m_world->GetComponent(entity, PhysicsBody::TypeID)
    );
    if (!body || body->bodyType != BodyType::Dynamic) return;
    
    ApplyImpulseInternal(body, impulse);
}

void PhysicsPlugin::SetVelocity(Entity entity, const float velocity[3]) {
    auto* body = static_cast<PhysicsBody*>(
        m_world->GetComponent(entity, PhysicsBody::TypeID)
    );
    if (!body) return;
    
    body->velocity[0] = velocity[0];
    body->velocity[1] = velocity[1];
    body->velocity[2] = velocity[2];
}

void PhysicsPlugin::AddExplosionForce(
    const float position[3],
    float force,
    float radius
) {
    auto& entities = m_world->GetAllEntities();
    
    for (const Entity& entity : entities) {
        auto* body = static_cast<PhysicsBody*>(
            m_world->GetComponent(entity, PhysicsBody::TypeID)
        );
        if (!body || body->bodyType != BodyType::Dynamic) continue;
        
        auto* transform = static_cast<TransformComponent*>(
            m_world->GetComponent(entity, TransformComponent::TypeID)
        );
        if (!transform) continue;
        
        float delta[3];
        Sub(transform->position, position, delta);
        float dist = Length(delta);
        
        if (dist < radius && dist > 0.001f) {
            float falloff = 1.0f - (dist / radius);
            float explosionForce[3];
            Normalize(delta, explosionForce);
            Scale(explosionForce, force * falloff, explosionForce);
            
            Add(body->force, explosionForce, body->force);
        }
    }
}

void PhysicsPlugin::SetGravity(const float gravity[3]) {
    m_gravity[0] = gravity[0];
    m_gravity[1] = gravity[1];
    m_gravity[2] = gravity[2];
}

bool PhysicsPlugin::ShouldCollide(
    uint32_t layerA, uint32_t maskA,
    uint32_t layerB, uint32_t maskB
) const {
    return (layerA & maskB) != 0 && (layerB & maskA) != 0;
}

void PhysicsPlugin::ApplyImpulseInternal(PhysicsBody* body, const float impulse[3]) {
    if (body->inverseMass > 0.0f) {
        float deltaV[3];
        Scale(impulse, body->inverseMass, deltaV);
        Add(body->velocity, deltaV, body->velocity);
    }
}

} // namespace SecretEngine::Physics

// Plugin factory implementation
extern "C" {
    SecretEngine::IPlugin* CreatePhysicsPlugin() {
        return new SecretEngine::Physics::PhysicsPlugin();
    }
    
    void DestroyPhysicsPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
