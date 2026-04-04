// SecretEngine
// Module: core
// Responsibility: Entity with component management and hierarchy
// Inspired by: Overload Engine's Actor pattern

#pragma once
#include <SecretEngine/IComponent.h>
#include <SecretEngine/Entity.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>

namespace SecretEngine {

/**
 * EntityObject represents a game object with components and hierarchy.
 * It manages component lifecycle, parent-child relationships, and active state.
 */
class EntityObject {
public:
    using ComponentPtr = std::shared_ptr<IComponent>;
    using ComponentCallback = std::function<void(IComponent&)>;
    
    EntityObject(Entity handle, const std::string& name = "Entity");
    ~EntityObject();
    
    // Component Management
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args);
    
    template<typename T>
    bool RemoveComponent();
    
    template<typename T>
    T* GetComponent() const;
    
    std::vector<ComponentPtr>& GetComponents() { return m_components; }
    const std::vector<ComponentPtr>& GetComponents() const { return m_components; }
    
    // Hierarchy
    void SetParent(EntityObject* parent);
    void DetachFromParent();
    EntityObject* GetParent() const { return m_parent; }
    const std::vector<EntityObject*>& GetChildren() const { return m_children; }
    bool IsDescendantOf(const EntityObject* entity) const;
    
    // State Management
    void SetActive(bool active);
    bool IsSelfActive() const { return m_active; }
    bool IsActive() const; // Considers parent hierarchy
    
    void MarkForDestruction() { m_markedForDestruction = true; }
    bool IsMarkedForDestruction() const { return m_markedForDestruction; }
    
    // Lifecycle (called by Scene/World)
    void OnAwake();
    void OnStart();
    void OnUpdate(float deltaTime);
    void OnFixedUpdate(float deltaTime);
    void OnLateUpdate(float deltaTime);
    void OnDisable();
    void OnDestroy();
    
    // Properties
    Entity GetHandle() const { return m_handle; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetTag() const { return m_tag; }
    void SetTag(const std::string& tag) { m_tag = tag; }
    
    // Callbacks for scene integration
    void SetComponentAddedCallback(ComponentCallback callback) {
        m_componentAddedCallback = callback;
    }
    void SetComponentRemovedCallback(ComponentCallback callback) {
        m_componentRemovedCallback = callback;
    }
    
private:
    void RecursiveActiveUpdate();
    void AddChild(EntityObject* child);
    void RemoveChild(EntityObject* child);
    
    Entity m_handle;
    std::string m_name;
    std::string m_tag;
    
    std::vector<ComponentPtr> m_components;
    std::unordered_map<uint32_t, IComponent*> m_componentMap; // Fast lookup by type ID
    
    EntityObject* m_parent = nullptr;
    std::vector<EntityObject*> m_children;
    
    bool m_active = true;
    bool m_awake = false;
    bool m_started = false;
    bool m_markedForDestruction = false;
    
    ComponentCallback m_componentAddedCallback;
    ComponentCallback m_componentRemovedCallback;
};

// Template implementations
template<typename T, typename... Args>
T* EntityObject::AddComponent(Args&&... args) {
    // Check if component already exists
    uint32_t typeID = ComponentTypeID<T>::Get();
    if (m_componentMap.find(typeID) != m_componentMap.end()) {
        return static_cast<T*>(m_componentMap[typeID]);
    }
    
    // Create component
    auto component = std::make_shared<T>(*this, std::forward<Args>(args)...);
    m_components.push_back(component);
    m_componentMap[typeID] = component.get();
    
    // Trigger callback
    if (m_componentAddedCallback) {
        m_componentAddedCallback(*component);
    }
    
    // Call lifecycle methods if entity is already initialized
    if (m_awake) {
        component->OnAwake();
        if (IsActive()) {
            component->OnEnable();
        }
        if (m_started) {
            component->OnStart();
        }
    }
    
    return component.get();
}

template<typename T>
bool EntityObject::RemoveComponent() {
    uint32_t typeID = ComponentTypeID<T>::Get();
    auto it = m_componentMap.find(typeID);
    if (it == m_componentMap.end()) {
        return false;
    }
    
    IComponent* comp = it->second;
    
    // Call lifecycle methods
    if (IsActive()) {
        comp->OnDisable();
    }
    comp->OnDestroy();
    
    // Trigger callback
    if (m_componentRemovedCallback) {
        m_componentRemovedCallback(*comp);
    }
    
    // Remove from map
    m_componentMap.erase(it);
    
    // Remove from vector
    m_components.erase(
        std::remove_if(m_components.begin(), m_components.end(),
            [comp](const ComponentPtr& ptr) { return ptr.get() == comp; }),
        m_components.end()
    );
    
    return true;
}

template<typename T>
T* EntityObject::GetComponent() const {
    uint32_t typeID = ComponentTypeID<T>::Get();
    auto it = m_componentMap.find(typeID);
    return it != m_componentMap.end() ? static_cast<T*>(it->second) : nullptr;
}

} // namespace SecretEngine
