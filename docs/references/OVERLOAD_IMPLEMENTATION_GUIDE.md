# Overload Implementation Patterns for SecretEngine

This guide provides concrete implementation patterns from Overload that can be adapted for SecretEngine.

## 1. Component Base Class Pattern

### Overload's Approach
```cpp
class AComponent : public API::IInspectorItem
{
public:
    AComponent(ECS::Actor& p_owner);
    virtual ~AComponent();
    
    // Lifecycle hooks (all virtual, default empty implementation)
    virtual void OnAwake() {}
    virtual void OnStart() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(float p_deltaTime) {}
    virtual void OnFixedUpdate(float p_deltaTime) {}
    virtual void OnLateUpdate(float p_deltaTime) {}
    
    // Physics callbacks
    virtual void OnCollisionEnter(Components::CPhysicalObject& p_otherObject) {}
    virtual void OnCollisionStay(Components::CPhysicalObject& p_otherObject) {}
    virtual void OnCollisionExit(Components::CPhysicalObject& p_otherObject) {}
    virtual void OnTriggerEnter(Components::CPhysicalObject& p_otherObject) {}
    virtual void OnTriggerStay(Components::CPhysicalObject& p_otherObject) {}
    virtual void OnTriggerExit(Components::CPhysicalObject& p_otherObject) {}
    
    // RTTI
    virtual std::string GetName() = 0;
    virtual std::string GetTypeName() = 0;
    
    ECS::Actor& owner;
};
```

### Adaptation for SecretEngine
```cpp
// core/include/SecretEngine/IComponent.h
namespace SecretEngine {

class Entity;

class IComponent {
public:
    explicit IComponent(Entity& owner) : m_owner(owner) {}
    virtual ~IComponent() = default;
    
    // Lifecycle
    virtual void OnAwake() {}
    virtual void OnStart() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float deltaTime) {}
    virtual void OnLateUpdate(float deltaTime) {}
    
    // Physics callbacks (when physics system is added)
    virtual void OnCollisionEnter(IComponent* other) {}
    virtual void OnCollisionStay(IComponent* other) {}
    virtual void OnCollisionExit(IComponent* other) {}
    
    // RTTI
    virtual const char* GetTypeName() const = 0;
    virtual uint32_t GetTypeID() const = 0;
    
    Entity& GetOwner() { return m_owner; }
    const Entity& GetOwner() const { return m_owner; }
    
protected:
    Entity& m_owner;
};

// Type registration helper
template<typename T>
struct ComponentTypeID {
    static uint32_t Get() {
        static uint32_t id = s_nextID++;
        return id;
    }
private:
    static inline uint32_t s_nextID = 0;
};

} // namespace SecretEngine
```

## 2. Entity (Actor) Pattern

### Overload's Actor Management
```cpp
class Actor {
public:
    // Component management with templates
    template<typename T, typename... Args>
    T& AddComponent(Args&&... p_args);
    
    template<typename T>
    bool RemoveComponent();
    
    template<typename T>
    T* GetComponent() const;
    
    std::vector<std::shared_ptr<Components::AComponent>>& GetComponents();
    
    // Hierarchy
    void SetParent(Actor& p_parent);
    void DetachFromParent();
    Actor* GetParent() const;
    std::vector<Actor*>& GetChildren();
    
    // State
    void SetActive(bool p_active);
    bool IsSelfActive() const;
    bool IsActive() const; // Considers parent hierarchy
    
    // Lifecycle propagation
    void OnAwake();
    void OnStart();
    void OnUpdate(float p_deltaTime);
    // ... etc
    
    // Events
    OvTools::Eventing::Event<Components::AComponent&> ComponentAddedEvent;
    OvTools::Eventing::Event<Components::AComponent&> ComponentRemovedEvent;
    
    // Always has transform
    Components::CTransform& transform;
    
private:
    std::vector<std::shared_ptr<Components::AComponent>> m_components;
    Actor* m_parent = nullptr;
    std::vector<Actor*> m_children;
    bool m_active = true;
    bool m_destroyed = false;
};
```

### Adaptation for SecretEngine
```cpp
// core/include/SecretEngine/Entity.h
namespace SecretEngine {

class Entity {
public:
    Entity(EntityID id, const std::string& name);
    ~Entity();
    
    // Component management
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        // Check if component already exists
        if (GetComponent<T>()) {
            return GetComponent<T>();
        }
        
        // Create component
        auto component = std::make_shared<T>(*this, std::forward<Args>(args)...);
        m_components.push_back(component);
        
        // Register in type map for fast lookup
        uint32_t typeID = ComponentTypeID<T>::Get();
        m_componentMap[typeID] = component.get();
        
        // Trigger event
        if (m_componentAddedCallback) {
            m_componentAddedCallback(*component);
        }
        
        // Call OnAwake if entity is already awake
        if (m_awake) {
            component->OnAwake();
            if (m_active) {
                component->OnEnable();
            }
        }
        
        return component.get();
    }
    
    template<typename T>
    bool RemoveComponent() {
        uint32_t typeID = ComponentTypeID<T>::Get();
        auto it = m_componentMap.find(typeID);
        if (it == m_componentMap.end()) {
            return false;
        }
        
        IComponent* comp = it->second;
        
        // Call lifecycle methods
        if (m_active) {
            comp->OnDisable();
        }
        comp->OnDestroy();
        
        // Trigger event
        if (m_componentRemovedCallback) {
            m_componentRemovedCallback(*comp);
        }
        
        // Remove from map
        m_componentMap.erase(it);
        
        // Remove from vector
        m_components.erase(
            std::remove_if(m_components.begin(), m_components.end(),
                [comp](const auto& ptr) { return ptr.get() == comp; }),
            m_components.end()
        );
        
        return true;
    }
    
    template<typename T>
    T* GetComponent() const {
        uint32_t typeID = ComponentTypeID<T>::Get();
        auto it = m_componentMap.find(typeID);
        return it != m_componentMap.end() ? static_cast<T*>(it->second) : nullptr;
    }
    
    // Hierarchy
    void SetParent(Entity* parent);
    void DetachFromParent();
    Entity* GetParent() const { return m_parent; }
    const std::vector<Entity*>& GetChildren() const { return m_children; }
    
    // State management
    void SetActive(bool active);
    bool IsSelfActive() const { return m_active; }
    bool IsActive() const; // Checks parent hierarchy
    
    void MarkForDestruction() { m_markedForDestruction = true; }
    bool IsMarkedForDestruction() const { return m_markedForDestruction; }
    
    // Lifecycle
    void OnAwake();
    void OnStart();
    void OnUpdate(float deltaTime);
    void OnFixedUpdate(float deltaTime);
    void OnLateUpdate(float deltaTime);
    void OnDisable();
    void OnDestroy();
    
    // Callbacks
    using ComponentCallback = std::function<void(IComponent&)>;
    void SetComponentAddedCallback(ComponentCallback callback) {
        m_componentAddedCallback = callback;
    }
    void SetComponentRemovedCallback(ComponentCallback callback) {
        m_componentRemovedCallback = callback;
    }
    
    // Properties
    EntityID GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetTag() const { return m_tag; }
    void SetTag(const std::string& tag) { m_tag = tag; }
    
private:
    void RecursiveActiveUpdate();
    
    EntityID m_id;
    std::string m_name;
    std::string m_tag;
    
    std::vector<std::shared_ptr<IComponent>> m_components;
    std::unordered_map<uint32_t, IComponent*> m_componentMap; // Fast lookup
    
    Entity* m_parent = nullptr;
    std::vector<Entity*> m_children;
    
    bool m_active = true;
    bool m_awake = false;
    bool m_started = false;
    bool m_markedForDestruction = false;
    
    ComponentCallback m_componentAddedCallback;
    ComponentCallback m_componentRemovedCallback;
};

} // namespace SecretEngine
```

## 3. Scene Pattern with Fast Access

### Overload's Scene System
```cpp
class Scene {
public:
    // Fast access to specific component types
    struct FastAccessComponents {
        std::vector<ECS::Components::CModelRenderer*> modelRenderers;
        std::vector<ECS::Components::CCamera*> cameras;
        std::vector<ECS::Components::CLight*> lights;
        std::vector<ECS::Components::CPostProcessStack*> postProcessStacks;
        std::vector<ECS::Components::CReflectionProbe*> reflectionProbes;
    };
    
    void Update(float p_deltaTime);
    void FixedUpdate(float p_deltaTime);
    void LateUpdate(float p_deltaTime);
    
    ECS::Actor& CreateActor(const std::string& p_name, const std::string& p_tag = "");
    bool DestroyActor(ECS::Actor& p_target);
    void CollectGarbages(); // Remove destroyed actors
    
    ECS::Actor* FindActorByName(const std::string& p_name) const;
    ECS::Actor* FindActorByTag(const std::string& p_tag) const;
    ECS::Actor* FindActorByID(int64_t p_id) const;
    
    // Component callbacks
    void OnComponentAdded(ECS::Components::AComponent& p_component);
    void OnComponentRemoved(ECS::Components::AComponent& p_component);
    
private:
    std::vector<ECS::Actor*> m_actors;
    FastAccessComponents m_fastAccessComponents;
    bool m_isPlaying = false;
};
```

### Adaptation for SecretEngine
```cpp
// core/include/SecretEngine/Scene.h
namespace SecretEngine {

class Scene {
public:
    Scene(const std::string& name);
    ~Scene();
    
    // Fast access for rendering/systems
    struct FastAccess {
        std::vector<MeshComponent*> meshes;
        std::vector<CameraComponent*> cameras;
        std::vector<LightComponent*> lights;
        std::vector<TransformComponent*> transforms;
    };
    
    // Entity management
    Entity* CreateEntity(const std::string& name = "Entity");
    bool DestroyEntity(Entity* entity);
    void CollectGarbage(); // Remove marked entities
    
    // Queries
    Entity* FindEntityByName(const std::string& name) const;
    Entity* FindEntityByTag(const std::string& tag) const;
    Entity* FindEntityByID(EntityID id) const;
    std::vector<Entity*> FindEntitiesByTag(const std::string& tag) const;
    
    // Lifecycle
    void Play();
    void Stop();
    bool IsPlaying() const { return m_isPlaying; }
    
    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);
    void LateUpdate(float deltaTime);
    
    // Fast access
    const FastAccess& GetFastAccess() const { return m_fastAccess; }
    
    // Serialization
    bool SaveToFile(const std::string& filepath);
    bool LoadFromFile(const std::string& filepath);
    
private:
    void OnComponentAdded(IComponent& component);
    void OnComponentRemoved(IComponent& component);
    
    void RegisterComponentForFastAccess(IComponent& component);
    void UnregisterComponentFromFastAccess(IComponent& component);
    
    std::string m_name;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<EntityID, Entity*> m_entityMap;
    std::unordered_map<std::string, std::vector<Entity*>> m_entitiesByTag;
    
    FastAccess m_fastAccess;
    bool m_isPlaying = false;
    EntityID m_nextEntityID = 1;
};

} // namespace SecretEngine
```

## 4. Transform Component Pattern

### Overload's Transform
```cpp
class CTransform : public AComponent {
public:
    // Local transform
    void SetLocalPosition(const OvMaths::FVector3& p_newPosition);
    void SetLocalRotation(const OvMaths::FQuaternion& p_newRotation);
    void SetLocalScale(const OvMaths::FVector3& p_newScale);
    
    // World transform (considers parent hierarchy)
    OvMaths::FVector3 GetWorldPosition() const;
    OvMaths::FQuaternion GetWorldRotation() const;
    OvMaths::FVector3 GetWorldScale() const;
    
    // Matrix generation
    OvMaths::FMatrix4 GetLocalMatrix() const;
    OvMaths::FMatrix4 GetWorldMatrix() const;
    
    // Direction vectors
    OvMaths::FVector3 GetWorldForward() const;
    OvMaths::FVector3 GetWorldUp() const;
    OvMaths::FVector3 GetWorldRight() const;
    
    // Hierarchy notification
    void NotificationHandler(ECS::Components::CTransform::ENotification p_notification);
    
private:
    void PreDecomposeWorldMatrix();
    void PreDecomposeLocalMatrix();
    
    OvMaths::FVector3 m_localPosition;
    OvMaths::FQuaternion m_localRotation;
    OvMaths::FVector3 m_localScale;
    
    OvMaths::FMatrix4 m_worldMatrix;
    OvMaths::FMatrix4 m_localMatrix;
};
```

### Key Insights
- Transform is mandatory on every entity
- Maintains both local and world transforms
- Caches matrices for performance
- Notifies children when parent transform changes
- Provides direction vectors (forward, up, right)

## 5. Event System Pattern

### Overload's Event System
```cpp
template<typename... ArgTypes>
class Event {
public:
    using Callback = std::function<void(ArgTypes...)>;
    
    ListenerID AddListener(Callback p_callback);
    bool RemoveListener(ListenerID p_listenerID);
    void RemoveAllListeners();
    uint64_t GetListenerCount() const;
    void Invoke(ArgTypes... p_args);
    
private:
    std::unordered_map<ListenerID, Callback> m_callbacks;
    ListenerID m_availableListenerID = 0;
};

// Usage in Actor:
OvTools::Eventing::Event<Components::AComponent&> ComponentAddedEvent;
ComponentAddedEvent.Invoke(component);
```

### Adaptation for SecretEngine
```cpp
// core/include/SecretEngine/Event.h
namespace SecretEngine {

template<typename... Args>
class Event {
public:
    using Callback = std::function<void(Args...)>;
    using ListenerID = uint64_t;
    
    ListenerID AddListener(Callback callback) {
        ListenerID id = m_nextID++;
        m_listeners[id] = callback;
        return id;
    }
    
    void RemoveListener(ListenerID id) {
        m_listeners.erase(id);
    }
    
    void RemoveAllListeners() {
        m_listeners.clear();
    }
    
    void Invoke(Args... args) {
        for (auto& [id, callback] : m_listeners) {
            callback(args...);
        }
    }
    
    size_t GetListenerCount() const {
        return m_listeners.size();
    }
    
private:
    std::unordered_map<ListenerID, Callback> m_listeners;
    ListenerID m_nextID = 0;
};

} // namespace SecretEngine
```

## 6. Resource Manager Pattern

### Overload's Approach
```cpp
template<typename T>
class ResourceManager {
public:
    T* Load(const std::string& p_path);
    void Unload(const std::string& p_path);
    void UnloadAll();
    T* Get(const std::string& p_path);
    bool IsLoaded(const std::string& p_path) const;
    
private:
    std::unordered_map<std::string, std::shared_ptr<T>> m_resources;
};
```

### Adaptation for SecretEngine
```cpp
// core/include/SecretEngine/ResourceManager.h
namespace SecretEngine {

template<typename T>
class ResourceManager {
public:
    std::shared_ptr<T> Load(const std::string& path) {
        auto it = m_resources.find(path);
        if (it != m_resources.end()) {
            return it->second; // Already loaded
        }
        
        auto resource = std::make_shared<T>();
        if (resource->LoadFromFile(path)) {
            m_resources[path] = resource;
            return resource;
        }
        
        return nullptr;
    }
    
    std::shared_ptr<T> Get(const std::string& path) {
        auto it = m_resources.find(path);
        return it != m_resources.end() ? it->second : nullptr;
    }
    
    void Unload(const std::string& path) {
        m_resources.erase(path);
    }
    
    void UnloadAll() {
        m_resources.clear();
    }
    
    bool IsLoaded(const std::string& path) const {
        return m_resources.find(path) != m_resources.end();
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<T>> m_resources;
};

// Specialized managers
class AssetManager {
public:
    ResourceManager<Mesh> meshes;
    ResourceManager<Texture> textures;
    ResourceManager<Shader> shaders;
    ResourceManager<Material> materials;
    ResourceManager<Sound> sounds;
};

} // namespace SecretEngine
```

## 7. Serialization Pattern

### Overload's ISerializable
```cpp
class ISerializable {
public:
    virtual void OnSerialize(tinyxml2::XMLDocument& p_doc, tinyxml2::XMLNode* p_root) = 0;
    virtual void OnDeserialize(tinyxml2::XMLDocument& p_doc, tinyxml2::XMLNode* p_root) = 0;
};
```

### Adaptation for SecretEngine (JSON-based)
```cpp
// core/include/SecretEngine/ISerializable.h
#include <nlohmann/json.hpp>

namespace SecretEngine {

class ISerializable {
public:
    virtual ~ISerializable() = default;
    
    virtual nlohmann::json Serialize() const = 0;
    virtual void Deserialize(const nlohmann::json& data) = 0;
};

// Example component implementation
class MeshComponent : public IComponent, public ISerializable {
public:
    nlohmann::json Serialize() const override {
        nlohmann::json j;
        j["type"] = "MeshComponent";
        j["meshPath"] = m_meshPath;
        j["visible"] = m_visible;
        return j;
    }
    
    void Deserialize(const nlohmann::json& data) override {
        if (data.contains("meshPath")) {
            m_meshPath = data["meshPath"];
            // Load mesh...
        }
        if (data.contains("visible")) {
            m_visible = data["visible"];
        }
    }
    
private:
    std::string m_meshPath;
    bool m_visible = true;
};

} // namespace SecretEngine
```

## Implementation Priority

1. **Phase 1: Core ECS**
   - IComponent base class with lifecycle hooks
   - Entity class with component management
   - ComponentTypeID system for RTTI

2. **Phase 2: Scene Management**
   - Scene class with entity management
   - Fast access component lists
   - Entity queries (by name, tag, ID)

3. **Phase 3: Transform System**
   - Transform component
   - Hierarchical transforms
   - Matrix caching

4. **Phase 4: Event System**
   - Generic Event<> template
   - Component add/remove events
   - Entity lifecycle events

5. **Phase 5: Resource Management**
   - ResourceManager<T> template
   - AssetManager facade
   - Reference counting

6. **Phase 6: Serialization**
   - ISerializable interface
   - JSON serialization for components
   - Scene save/load

## Testing Strategy

1. Create simple test components (e.g., HealthComponent, PositionComponent)
2. Test component add/remove/get operations
3. Test entity hierarchy (parent/child relationships)
4. Test active state propagation
5. Test lifecycle method calls
6. Test scene serialization/deserialization
7. Test resource loading and caching

## Next Steps

1. Review current SecretEngine Entity implementation
2. Identify gaps compared to Overload patterns
3. Implement IComponent base class
4. Refactor existing components to inherit from IComponent
5. Add lifecycle hooks to components
6. Implement event system
7. Add fast access component lists to World/Scene
8. Implement resource manager
9. Add serialization support
