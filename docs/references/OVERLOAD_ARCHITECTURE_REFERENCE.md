# Overload Engine Architecture Reference

This document provides architectural insights from the Overload game engine (https://github.com/kmqwerty/Overload) to inspire SecretEngine development.

## Overview

Overload is a free, open-source 3D game engine built with C++20 and Lua scripting. It follows a modular SDK architecture with clear separation of concerns.

## Core Architecture Principles

### 1. Modular SDK Design
Overload divides functionality into 11 modules: 9 libraries (SDK) + 2 executables (Applications)

**SDK Modules:**
- **OvDebug**: Logging and assertions
- **OvTools**: Serialization, file system, platform, events, clock
- **OvMaths**: Vectors, matrices, quaternions, transforms
- **OvAudio**: Audio engine (built on SoLoud)
- **OvPhysics**: Physics engine (built on Bullet3)
- **OvRendering**: Hardware Abstraction Layer (HAL) for rendering with OpenGL implementation
- **OvWindowing**: Input and window management (GLFW)
- **OvUI**: Widget-based UI (leveraging ImGui)
- **OvCore**: Component-based scene system, scripting, resource management

**Applications:**
- **OvGame**: Data-driven executable for games
- **OvEditor**: Game editor

### 2. Entity-Component-System (ECS) Architecture

#### Actor (Entity)
The Actor class is the core entity in Overload's ECS:

```cpp
class Actor : public API::ISerializable
{
public:
    // Lifecycle methods
    void OnAwake();
    void OnStart();
    void OnEnable();
    void OnDisable();
    void OnDestroy();
    void OnUpdate(float p_deltaTime);
    void OnFixedUpdate(float p_deltaTime);
    void OnLateUpdate(float p_deltaTime);
    
    // Physics callbacks
    void OnCollisionEnter(Components::CPhysicalObject& p_otherObject);
    void OnCollisionStay(Components::CPhysicalObject& p_otherObject);
    void OnCollisionExit(Components::CPhysicalObject& p_otherObject);
    void OnTriggerEnter(Components::CPhysicalObject& p_otherObject);
    void OnTriggerStay(Components::CPhysicalObject& p_otherObject);
    void OnTriggerExit(Components::CPhysicalObject& p_otherObject);
    
    // Component management
    template<typename T, typename ... Args>
    T& AddComponent(Args&&... p_args);
    
    template<typename T>
    bool RemoveComponent();
    
    template<typename T>
    T* GetComponent() const;
    
    // Behaviour (script) management
    Components::Behaviour& AddBehaviour(const std::string& p_name);
    bool RemoveBehaviour(Components::Behaviour& p_behaviour);
    Components::Behaviour* GetBehaviour(const std::string& p_name);
    
    // Hierarchy
    void SetParent(Actor& p_parent);
    void DetachFromParent();
    Actor* GetParent() const;
    std::vector<Actor*>& GetChildren();
    
    // State management
    void SetActive(bool p_active);
    bool IsSelfActive() const;
    bool IsActive() const;
    void MarkAsDestroy();
    bool IsAlive() const;
    
    // Events
    OvTools::Eventing::Event<Components::AComponent&> ComponentAddedEvent;
    OvTools::Eventing::Event<Components::AComponent&> ComponentRemovedEvent;
    OvTools::Eventing::Event<Components::Behaviour&> BehaviourAddedEvent;
    OvTools::Eventing::Event<Components::Behaviour&> BehaviourRemovedEvent;
    
    static OvTools::Eventing::Event<Actor&> DestroyedEvent;
    static OvTools::Eventing::Event<Actor&> CreatedEvent;
    static OvTools::Eventing::Event<Actor&, Actor&> AttachEvent;
    static OvTools::Eventing::Event<Actor&> DettachEvent;
    
    // Transform is always present
    Components::CTransform& transform;
};
```

**Key Insights:**
- Every Actor has a mandatory Transform component
- Lifecycle is well-defined: OnAwake → OnEnable → OnStart → OnUpdate/OnFixedUpdate/OnLateUpdate → OnDisable → OnDestroy
- Hierarchical parent-child relationships
- Active state propagates through hierarchy
- Event-driven architecture for component/behaviour changes
- Sleeping state for optimization (doesn't trigger lifecycle methods)

#### Component Types

**Rendering Components:**
- CCamera
- CModelRenderer
- CMaterialRenderer
- CPostProcessStack
- CReflectionProbe

**Lighting Components:**
- CLight (base)
- CDirectionalLight
- CPointLight
- CSpotLight
- CAmbientBoxLight
- CAmbientSphereLight

**Physics Components:**
- CPhysicalObject (base)
- CPhysicalBox
- CPhysicalSphere
- CPhysicalCapsule

**Audio Components:**
- CAudioSource
- CAudioListener

**Core Components:**
- CTransform (mandatory on every Actor)
- Behaviour (Lua script component)

### 3. Resource Management

Overload uses a centralized resource management system in OvCore:
- **ResourceManagement**: Handles loading, caching, and lifecycle
- **Resources**: Defines resource types (Model, Texture, Shader, Material, Sound, etc.)
- Resource references use shared pointers for automatic memory management

### 4. Rendering Architecture

**Hardware Abstraction Layer (HAL):**
- OvRendering provides an agnostic rendering interface
- OpenGL implementation using GLAD
- Supports PBR (Physically-Based Rendering)
- Custom shader support
- Material system with property editing

### 5. Scripting System

**Lua Integration:**
- Sol3 library for C++ ↔ Lua binding
- Behaviour component wraps Lua scripts
- Scripts have access to full Actor API
- Lifecycle methods exposed to Lua

### 6. Scene System

**Scene Management:**
- Scenes contain Actors
- Serialization/Deserialization using tinyxml2
- Scene loading/unloading
- Play mode vs Edit mode distinction

### 7. Build System

**Premake5:**
- Cross-platform project generation
- Modular dependency management
- Separate configurations for Debug/Release
- Platform-specific toolset selection (MSVC on Windows, Clang on Linux)

## Key Patterns to Adopt for SecretEngine

### 1. Clear Module Boundaries
```
SecretEngine/
├── core/           # Core ECS, entity management
├── rendering/      # Rendering abstraction
├── physics/        # Physics integration
├── audio/          # Audio system
├── tools/          # Utilities, serialization
├── ui/             # UI system
└── plugins/        # Plugin system (already exists)
```

### 2. Component Lifecycle
Implement clear lifecycle hooks:
- OnAwake (initialization)
- OnEnable (activation)
- OnStart (first frame)
- OnUpdate (every frame)
- OnFixedUpdate (physics frame)
- OnLateUpdate (after update)
- OnDisable (deactivation)
- OnDestroy (cleanup)

### 3. Event-Driven Architecture
Use events for:
- Component addition/removal
- Entity creation/destruction
- Parent-child attachment
- State changes

### 4. Hierarchical Entities
- Parent-child relationships
- Transform hierarchy
- Active state propagation
- Recursive operations

### 5. Template-Based Component System
```cpp
template<typename T, typename... Args>
T& Entity::AddComponent(Args&&... args) {
    // Check if component already exists
    // Create and register component
    // Trigger ComponentAddedEvent
    // Return reference
}
```

### 6. Resource Caching
- Centralized resource manager
- Reference counting with shared_ptr
- Lazy loading
- Automatic cleanup

### 7. Serialization Interface
```cpp
class ISerializable {
public:
    virtual void OnSerialize(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root) = 0;
    virtual void OnDeserialize(tinyxml2::XMLDocument& doc, tinyxml2::XMLNode* root) = 0;
};
```

## Dependencies Used by Overload

- **GLAD**: OpenGL loader
- **GLFW**: Windowing and input
- **Assimp**: 3D model loading
- **Bullet3**: Physics simulation
- **SoLoud**: Audio engine
- **Tinyxml2**: XML serialization
- **Sol3**: Lua binding
- **ImGui**: Immediate mode GUI
- **Tracy**: Profiling (with on-demand mode)
- **Premake5**: Build system

## Performance Considerations

1. **Memory Management**:
   - Shared pointers for resources
   - Component storage in vectors
   - Behaviour storage in unordered_map

2. **Profiling**:
   - Tracy integration for performance monitoring
   - On-demand profiling mode
   - Memory tracking enabled

3. **Optimization Flags**:
   - Edit and Continue disabled (incompatible with Tracy)
   - Platform-specific toolsets
   - Debug/Release configurations

## Comparison with SecretEngine

### Similarities
- Plugin-based architecture
- Component system
- JSON-based data files
- Cross-platform support

### Differences
- Overload uses Lua scripting; SecretEngine uses C++ plugins
- Overload has built-in editor; SecretEngine uses Blender addon
- Overload uses OpenGL; SecretEngine uses Vulkan
- Overload uses Bullet3; SecretEngine physics TBD

### Opportunities for SecretEngine

1. **Adopt ECS Lifecycle**: Implement OnAwake/OnStart/OnUpdate/OnDisable/OnDestroy pattern
2. **Event System**: Add event-driven component management
3. **Hierarchical Entities**: Implement parent-child relationships with transform hierarchy
4. **Resource Manager**: Centralized resource loading and caching
5. **Serialization Interface**: Standardize save/load across components
6. **Component Templates**: Type-safe component addition/removal
7. **Active State Propagation**: Hierarchical enable/disable
8. **Physics Callbacks**: OnCollision/OnTrigger events
9. **Profiling Integration**: Add Tracy or similar profiler
10. **Material System**: Separate material from mesh rendering

## Recommended Next Steps

1. Review current SecretEngine Entity/Component implementation
2. Design lifecycle system for components
3. Implement event system for entity/component changes
4. Add hierarchical transform system
5. Create resource manager for meshes/textures/shaders
6. Standardize serialization across all components
7. Add physics callback system
8. Implement active state propagation
9. Consider adding profiling hooks
10. Document component API for plugin developers

## References

- Overload Repository: https://github.com/kmqwerty/Overload
- Overload Documentation: https://overloadengine.org/
- Original Authors: Benjamin VIRANIN, Max BRUN, Adrien GIVRY (2019)
- License: MIT
