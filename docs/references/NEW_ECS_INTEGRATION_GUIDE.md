# New ECS System Integration Guide

This guide explains how to integrate the new Overload-inspired ECS system into SecretEngine.

## Overview

The new system provides:
- **Component Lifecycle**: OnAwake, OnStart, OnUpdate, OnFixedUpdate, OnLateUpdate, OnDisable, OnDestroy
- **Entity Hierarchy**: Parent-child relationships with transform propagation
- **Fast Component Access**: Pre-sorted component lists for rendering
- **Event System**: Generic event callbacks for decoupled communication
- **Active State Propagation**: Hierarchical enable/disable
- **Type-Safe Components**: Template-based component management

## Architecture

```
Scene
├── EntityObject (Actor)
│   ├── Components (IComponent)
│   │   ├── TransformComponentNew
│   │   ├── MeshComponentNew
│   │   ├── CameraComponentNew
│   │   ├── LightComponentNew
│   │   └── Custom Components
│   └── Children (EntityObject)
└── FastAccess
    ├── meshes[]
    ├── cameras[]
    ├── lights[]
    └── transforms[]
```

## Key Classes

### IComponent
Base class for all components with lifecycle hooks.

```cpp
class IComponent {
    virtual void OnAwake() {}
    virtual void OnStart() {}
    virtual void OnUpdate(float deltaTime) {}
    // ... more lifecycle methods
};
```

### EntityObject
Manages components and hierarchy (equivalent to Overload's Actor).

```cpp
auto* entity = scene.CreateEntity("MyEntity");
auto* transform = entity->AddComponent<TransformComponentNew>();
auto* mesh = entity->AddComponent<MeshComponentNew>();
entity->SetParent(parentEntity);
```

### Scene
Manages entity lifecycle and provides fast component access.

```cpp
Scene scene("MyScene");
scene.Play();
scene.Update(deltaTime);
const auto& fastAccess = scene.GetFastAccess();
```

## Migration Path

### Phase 1: Parallel Systems (Current)
- Old World/Entity system continues to work
- New Scene/EntityObject system available
- Plugins can use either system

### Phase 2: Gradual Migration
1. Create new components inheriting from IComponent
2. Migrate rendering to use Scene::FastAccess
3. Update plugins to use EntityObject
4. Add lifecycle hooks to existing systems

### Phase 3: Full Integration
1. Replace World with Scene in ICore
2. Update all plugins to new system
3. Remove old Entity/Component POD structures
4. Update serialization to use new system

## Creating Custom Components

```cpp
class MyComponent : public IComponent {
public:
    explicit MyComponent(EntityObject& owner, int value)
        : IComponent(owner)
        , m_value(value)
    {}
    
    COMPONENT_TYPE(MyComponent) // Generates GetTypeName() and GetTypeID()
    
    void OnStart() override {
        // Called once when scene starts
    }
    
    void OnUpdate(float deltaTime) override {
        // Called every frame
    }
    
    void SetValue(int value) { m_value = value; }
    int GetValue() const { return m_value; }
    
private:
    int m_value;
};

// Usage:
auto* myComp = entity->AddComponent<MyComponent>(42);
myComp->SetValue(100);
```

## Using Events

```cpp
// Define event
Event<int, std::string> onScoreChanged;

// Add listener
auto listenerId = onScoreChanged.AddListener([](int score, std::string player) {
    std::cout << player << " scored " << score << std::endl;
});

// Invoke event
onScoreChanged.Invoke(100, "Player1");

// Remove listener
onScoreChanged.RemoveListener(listenerId);
```

## Entity Hierarchy

```cpp
// Create parent
auto* parent = scene.CreateEntity("Parent");
auto* parentTransform = parent->AddComponent<TransformComponentNew>();
parentTransform->SetLocalPosition(Vec3(10, 0, 0));

// Create child
auto* child = scene.CreateEntity("Child");
auto* childTransform = child->AddComponent<TransformComponentNew>();
childTransform->SetLocalPosition(Vec3(5, 0, 0));
child->SetParent(parent);

// Child's world position is parent + local = (15, 0, 0)
Vec3 worldPos = childTransform->GetWorldPosition();
```

## Fast Component Access

Instead of iterating all entities to find meshes:

```cpp
// Old way (slow):
for (auto entity : world->GetAllEntities()) {
    auto* mesh = world->GetComponent(entity, MeshComponent::TypeID);
    if (mesh) {
        // Render mesh
    }
}

// New way (fast):
const auto& fastAccess = scene.GetFastAccess();
for (auto* mesh : fastAccess.meshes) {
    if (mesh->IsVisible()) {
        auto* transform = mesh->GetOwner().GetComponent<TransformComponentNew>();
        // Render mesh with transform
    }
}
```

## Lifecycle Order

```
Scene.Play()
  └─> Entity.OnAwake()
      └─> Component.OnAwake()
  └─> Entity.OnEnable()
      └─> Component.OnEnable()
  └─> Entity.OnStart()
      └─> Component.OnStart()

Scene.Update(dt)
  └─> Entity.OnUpdate(dt)
      └─> Component.OnUpdate(dt)

Scene.FixedUpdate(dt)
  └─> Entity.OnFixedUpdate(dt)
      └─> Component.OnFixedUpdate(dt)

Scene.LateUpdate(dt)
  └─> Entity.OnLateUpdate(dt)
      └─> Component.OnLateUpdate(dt)

Entity.SetActive(false)
  └─> Component.OnDisable()

Entity.MarkForDestruction()
Scene.CollectGarbage()
  └─> Component.OnDestroy()
  └─> Entity destroyed
```

## Integration with Existing Systems

### Rendering Plugin
```cpp
// In renderer update:
const auto& fastAccess = scene.GetFastAccess();

// Render all visible meshes
for (auto* mesh : fastAccess.meshes) {
    if (!mesh->IsVisible()) continue;
    
    auto* transform = mesh->GetOwner().GetComponent<TransformComponentNew>();
    if (!transform) continue;
    
    Mat4 worldMatrix = transform->GetWorldMatrix();
    // Submit to renderer...
}

// Find main camera
auto* camera = scene.FindMainCamera();
if (camera) {
    Mat4 viewMatrix = camera->GetViewMatrix();
    Mat4 projMatrix = camera->GetProjectionMatrix(aspectRatio);
    // Set camera matrices...
}
```

### Physics Plugin
```cpp
// Add physics callbacks to IComponent:
class PhysicsComponent : public IComponent {
    void OnCollisionEnter(IComponent* other) override {
        // Handle collision
    }
};
```

### Level System Plugin
```cpp
// Load level into scene:
Scene* LoadLevel(const std::string& levelPath) {
    auto scene = new Scene(levelPath);
    
    // Parse JSON level data
    // Create entities with components
    auto* entity = scene->CreateEntity("Player");
    auto* transform = entity->AddComponent<TransformComponentNew>();
    auto* mesh = entity->AddComponent<MeshComponentNew>();
    
    return scene;
}
```

## Performance Considerations

### Fast Access Lists
- Pre-sorted component arrays avoid entity iteration
- O(1) access to all meshes, cameras, lights
- Automatically maintained by Scene

### Component Lookup
- O(1) component lookup by type using hash map
- No need to iterate component vector

### Hierarchy Updates
- Only root entities are updated by Scene
- Children are updated recursively by parent
- Active state propagates through hierarchy

### Memory
- Components use shared_ptr for automatic cleanup
- Fast access lists store raw pointers (no overhead)
- Entity destruction is deferred (CollectGarbage)

## Testing

Run the example:
```bash
# Build example (add to CMakeLists.txt)
add_executable(ECS_Example docs/examples/NEW_ECS_EXAMPLE.cpp)
target_link_libraries(ECS_Example SecretEngine_Core)

# Run
./ECS_Example
```

## Next Steps

1. **Add Scene to ICore**
   - Add `IScene* GetScene()` to ICore interface
   - Create Scene in Core initialization

2. **Update Renderer**
   - Use Scene::FastAccess instead of World iteration
   - Support TransformComponentNew matrices

3. **Migrate Level System**
   - Load levels into Scene instead of World
   - Create EntityObjects with new components

4. **Add Physics Integration**
   - Implement OnCollision callbacks
   - Create PhysicsComponent

5. **Serialization**
   - Implement Scene::SaveToFile/LoadFromFile
   - Use JSON for entity/component data

6. **Documentation**
   - Document all component types
   - Create tutorials for common patterns
   - Add API reference

## Compatibility

The new system is designed to coexist with the old system:
- Old World/Entity still works
- New Scene/EntityObject is separate
- Gradual migration is possible
- No breaking changes to existing code

## Benefits

✅ Clear component lifecycle  
✅ Hierarchical entities  
✅ Fast rendering access  
✅ Type-safe components  
✅ Event-driven architecture  
✅ Active state propagation  
✅ Extensible component system  
✅ Better code organization  

## Questions?

See:
- `docs/references/OVERLOAD_ARCHITECTURE_REFERENCE.md` - Architecture overview
- `docs/references/OVERLOAD_IMPLEMENTATION_GUIDE.md` - Implementation patterns
- `docs/examples/NEW_ECS_EXAMPLE.cpp` - Working examples
