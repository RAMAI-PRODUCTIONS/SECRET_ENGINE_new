SecretEngine – Scene Data Model (FROZEN)

Purpose

This document defines the EXACT structure of entities, components, and scenes at runtime.
This is binding. No alternatives are allowed.

## 1. Entity Definition

An Entity is NOT an object. It is an identifier.

```cpp
// Entity is just a handle
struct Entity {
    uint32_t id;
    uint32_t generation; // for handle reuse safety
};
```

Rules:
- Entities have no behavior
- Entities have no virtual methods
- Entities are POD (plain old data)
- Entities are copyable
- Entity(0,0) is always invalid

## 2. Component Definition

A Component is pure data attached to an entity.

### Component Rules (STRICT)
- Components are POD structs
- No virtual methods
- No destructors with side effects
- No heap allocations inside components
- No references to other components
- No pointers (use handles)

### Component Example
```cpp
struct TransformComponent {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    Entity parent;  // handle, not pointer
};

struct RenderableComponent {
    MeshHandle mesh;
    MaterialHandle material;
    uint8_t lod_bias;
    uint8_t visibility_flags;
};
```

## 3. Component Storage Strategy

Components are stored in SEPARATE ARRAYS per type (SoA - Structure of Arrays)

```
Scene:
  TransformComponents: [T0, T1, T2, ...]
  RenderableComponents: [R0, R1, ...]
  PhysicsComponents: [P0, P1, ...]
  
Entity-to-Component mapping via sparse set or hash map
```

Why SoA:
- Cache-friendly iteration
- Easy to add/remove component types
- No fragmentation
- SIMD-friendly

## 4. Scene Definition

A Scene is a collection of entities and their components.

### Scene Structure (Runtime Binary Format)
```
Scene {
    Header {
        uint32_t version;
        uint32_t entity_count;
        uint32_t component_type_count;
    }
    
    EntityTable {
        Entity entities[entity_count];
    }
    
    ComponentTypeBlock[] {
        uint32_t type_id;
        uint32_t component_count;
        uint32_t component_size;
        byte data[component_count * component_size];
    }
}
```

### Scene Rules
- Scenes are immutable after loading
- Scenes can be merged (not split)
- Scenes reference assets by handle, not by path
- Scenes contain no logic

## 5. World Definition

A World is the active runtime container.

```
World = Core::IWorld {
    + active scenes[]
    + entity allocator
    + component storage
    + queries
}
```

World responsibilities:
- Create/destroy entities
- Add/remove components
- Execute queries
- Manage scene lifecycle

World does NOT:
- Render
- Handle input
- Run physics
- Know about Vulkan

## 6. Component Query System

Queries select entities by component presence.

Example query:
```cpp
// "Give me all entities with Transform AND Renderable"
Query {
    required: [TransformComponent, RenderableComponent]
    excluded: [DisabledTag]
}
```

Query results:
- Iterators over matching entities
- Direct component array access (cache-friendly)
- Updated automatically when components change

## 7. Entity Lifecycle

```
1. Create Entity → Allocate ID
2. Add Components → Store in component arrays
3. Entity Active → Visible to queries
4. Remove Components → Mark slots free
5. Destroy Entity → Return ID to pool
```

No garbage collection. Explicit lifetime.

## 8. Component Registration

Component types are registered at plugin load time.

```cpp
Plugin::OnLoad() {
    core->RegisterComponentType<TransformComponent>("Transform");
    core->RegisterComponentType<RenderableComponent>("Renderable");
}
```

Core maintains a type registry. Plugins cannot invent types at runtime.

### Special Entity Types (Bootstrap)
- **PlayerStart**: Defines the initial camera spawn point (Position/Rotation).

## 9. Authoring JSON → Runtime Binary

### Authoring Scene (JSON)
```json
{
  "entities": [
    {
      "id": "player",
      "components": {
        "Transform": {
          "position": [0, 0, 0],
          "rotation": [0, 0, 0, 1],
          "scale": [1, 1, 1]
        },
        "Renderable": {
          "mesh": "assets/player.mesh",
          "material": "assets/player.mat"
        }
      }
    }
  ]
}
```

### Cooker Process
1. Parse JSON
2. Resolve asset paths → handles
3. Validate component schemas
4. Flatten to binary scene format
5. Write .scenebin

### Runtime Loading
1. Read .scenebin header
2. Allocate entities
3. Deserialize component arrays
4. Register with queries

No JSON parsing at runtime. Ever.

**Note**: During the bootstrap/development phase, the engine allows loading from `scene.json` for rapid iteration. This will be replaced by the binary system in production.

## 10. Memory Layout Guarantees

Component arrays are:
- Contiguous
- Aligned (16-byte minimum)
- Stable pointers during frame
- Relocatable between frames

Iteration order:
- Not guaranteed
- Not sorted by entity ID
- Optimized for cache, not logic

## 11. Tags vs Components

### Components
- Have data (size > 0)
- Stored in arrays
- Queried

### Tags
- No data (size = 0)
- Stored in bitsets
- Cheap presence checks

Example tags:
- DisabledTag
- StaticTag
- PlayerTag

## 12. Cross-Scene References

Entities can reference entities in other scenes via:
```cpp
struct EntityReference {
    SceneHandle scene;
    Entity entity;
};
```

Core validates references at load time.

## 13. Prefab System (Future-Proofing)

Prefabs = scenes with template entities.

Instantiation:
1. Load prefab scene
2. Clone entities
3. Remap handles
4. Merge into active scene

Not implemented initially, but data model supports it.

## 14. Forbidden Patterns

❌ Components storing pointers to other components
❌ Components with virtual methods
❌ Entities with behavior
❌ Runtime component type creation
❌ String-based component lookup in hot paths
❌ Per-entity update loops

## 15. What Plugins See

Plugins receive:
- Component queries
- Stable array pointers (during frame)
- Entity handles

Plugins do NOT see:
- Internal storage implementation
- Entity IDs directly (use queries)

## 16. Performance Assumptions

Assumptions this data model makes:
- 10,000+ entities per scene (mobile)
- 50,000+ entities per scene (desktop)
- Cache-friendly iteration is critical
- Random access is expensive
- Component addition/removal is infrequent

## 17. Validation Rules

Scene loader must validate:
- All component types are registered
- All asset handles are valid
- No circular entity references
- No orphaned components

Invalid scenes are rejected at load time, not runtime.

## 18. Debug Features

Debug builds include:
- Entity name strings (stripped in release)
- Component type names (stripped in release)
- Validation checks (stripped in release)

Release builds are pure data.

Status

✅ FROZEN
This is the data model. Implementation must match exactly.
