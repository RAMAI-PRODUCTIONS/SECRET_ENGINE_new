# SecretEngine Physics Plugin - Complete Feature Set

## Overview
Comprehensive physics system inspired by Unreal Engine and Jolt Physics, optimized for mobile FPS games.

## Features Implemented

### 1. Collision Shapes
- **Sphere** - Fast, simple collision (characters, projectiles)
- **Box (AABB)** - Walls, floors, obstacles
- **Capsule** - Character controllers (simplified as extended sphere)

### 2. Body Types
- **Static** - Immovable objects (walls, floors, environment)
- **Dynamic** - Fully simulated rigidbodies (physics objects, ragdolls)
- **Kinematic** - Movable but not affected by forces (character controllers)

### 3. Physics Simulation
- **Fixed timestep** (60Hz) with accumulator
- **Velocity integration** with forces and acceleration
- **Position integration** with velocity
- **Gravity** - Configurable per-body and global
- **Damping** - Air resistance simulation
- **Mass and inverse mass** - Proper physics calculations

### 4. Collision Detection
- **Broad phase** - All-pairs check (optimizable with spatial partitioning later)
- **Narrow phase** - Shape-specific collision tests:
  - Sphere vs Sphere
  - Box vs Box
  - Sphere vs Box
  - Capsule support (simplified)
- **Collision filtering** - Layer-based collision masks

### 5. Collision Response
- **Impulse-based resolution** - Realistic collision response
- **Restitution** - Bounciness (0 = no bounce, 1 = perfect bounce)
- **Friction** - Surface friction simulation
- **Penetration correction** - Separates overlapping objects
- **Relative velocity** - Proper collision response based on velocities

### 6. Raycasting
- **Basic raycast** - Origin, direction, max distance
- **Layered raycast** - Filter by collision layers
- **Shape-specific** - Ray vs Sphere, Box, Capsule
- **Hit information** - Point, normal, distance, entity

### 7. Queries
- **CheckGround** - Ground detection for character controllers
- **OverlapSphere** - Find entities in sphere radius
- **OverlapBox** - Find entities in box volume

### 8. Forces & Impulses
- **AddForce** - Continuous force application
- **AddImpulse** - Instant velocity change
- **SetVelocity** - Direct velocity control
- **AddExplosionForce** - Radial force with falloff

### 9. Constraints
- **Freeze Position** - Lock X, Y, or Z axis movement
- **Lock Rotation** - Prevent rotation
- **Use Gravity** - Toggle gravity per-body

### 10. Collision Layers
Predefined layers for filtering:
- Default
- Player
- Enemy
- Projectile
- Environment
- Trigger

### 11. Physics Materials
- **Friction** - Surface friction coefficient
- **Restitution** - Bounciness
- **Density** - Mass calculation

### 12. Triggers
- **isTrigger** flag - Collision detection without response
- Perfect for pickup zones, damage volumes, etc.

## Usage Examples

### Creating a Physics Body

```cpp
// Add physics body component
auto* body = new PhysicsBody();
body->bodyType = BodyType::Dynamic;
body->shapeType = ShapeType::Sphere;
body->shapeData[0] = 0.5f; // radius
body->mass = 1.0f;
body->inverseMass = 1.0f;
body->useGravity = true;
body->collisionLayer = CollisionLayer::Player;
body->collisionMask = CollisionLayer::All;
world->AddComponent(entity, PhysicsBody::TypeID, body);
```

### Raycasting

```cpp
float origin[3] = {0, 1, 0};
float direction[3] = {0, 0, 1};
auto hit = physics->Raycast(origin, direction, 100.0f);

if (hit.hit) {
    // Hit entity: hit.entityId
    // Hit point: hit.point
    // Hit normal: hit.normal
    // Distance: hit.distance
}
```

### Applying Forces

```cpp
// Continuous force
float force[3] = {0, 10, 0}; // Upward force
physics->AddForce(entity, force);

// Instant impulse
float impulse[3] = {5, 0, 0}; // Sideways push
physics->AddImpulse(entity, impulse);

// Explosion
float explosionPos[3] = {0, 0, 0};
physics->AddExplosionForce(explosionPos, 1000.0f, 10.0f);
```

### Collision Filtering

```cpp
// Player only collides with environment and enemies
body->collisionLayer = CollisionLayer::Player;
body->collisionMask = CollisionLayer::Environment | CollisionLayer::Enemy;

// Projectile collides with everything except other projectiles
body->collisionLayer = CollisionLayer::Projectile;
body->collisionMask = CollisionLayer::All & ~CollisionLayer::Projectile;
```

## Performance Characteristics

- **Fixed timestep**: Deterministic, consistent simulation
- **Substeps**: Up to 4 substeps per frame to prevent tunneling
- **Simple shapes**: Fast collision detection
- **No heap allocations** in hot path
- **Mobile-optimized**: Designed for 60 FPS on mobile devices

## Future Enhancements (Not Implemented)

- Spatial partitioning (Octree/Grid) for broad phase
- Continuous collision detection (CCD) for fast-moving objects
- Joints and constraints (hinges, springs)
- Cloth simulation
- Soft body physics
- Character controller with slope handling
- Physics debug visualization

## Integration with FPS Game

The physics system integrates seamlessly with the FPS game:
- Player and bots use PhysicsBody for collision
- Weapons use raycasting for hit detection
- Projectiles can use Dynamic bodies
- Environment uses Static bodies
- Triggers for pickup zones and objectives

## Architecture Compliance

✅ Plugin-based - No core dependencies
✅ POD components - PhysicsBody is pure data
✅ Stateless systems - All logic in plugin
✅ Fast Data compatible - Can use for collision events
✅ Mobile-first - Optimized for performance
✅ Modular - Easy to extend or replace
