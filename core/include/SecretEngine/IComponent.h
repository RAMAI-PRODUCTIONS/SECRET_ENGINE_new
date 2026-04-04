// SecretEngine
// Module: core
// Responsibility: Base interface for all components with lifecycle hooks
// Inspired by: Overload Engine's AComponent pattern

#pragma once
#include <cstdint>
#include <string>

namespace SecretEngine {

class EntityObject; // Forward declaration

/**
 * IComponent is the base class for all components in SecretEngine.
 * It provides lifecycle hooks that are called by the entity system.
 * 
 * Lifecycle order:
 * 1. OnAwake()     - Called once when component is first created
 * 2. OnEnable()    - Called when component/entity becomes active
 * 3. OnStart()     - Called on first frame after OnAwake
 * 4. OnUpdate()    - Called every frame
 * 5. OnFixedUpdate() - Called at fixed physics timestep
 * 6. OnLateUpdate() - Called after all OnUpdate calls
 * 7. OnDisable()   - Called when component/entity becomes inactive
 * 8. OnDestroy()   - Called before component is destroyed
 */
class IComponent {
public:
    explicit IComponent(EntityObject& owner) : m_owner(owner) {}
    virtual ~IComponent() = default;
    
    // Lifecycle hooks (override as needed)
    virtual void OnAwake() {}
    virtual void OnStart() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnFixedUpdate(float deltaTime) {}
    virtual void OnLateUpdate(float deltaTime) {}
    
    // Physics callbacks (for future physics integration)
    virtual void OnCollisionEnter(IComponent* other) {}
    virtual void OnCollisionStay(IComponent* other) {}
    virtual void OnCollisionExit(IComponent* other) {}
    virtual void OnTriggerEnter(IComponent* other) {}
    virtual void OnTriggerStay(IComponent* other) {}
    virtual void OnTriggerExit(IComponent* other) {}
    
    // RTTI (Run-Time Type Information)
    virtual const char* GetTypeName() const = 0;
    virtual uint32_t GetTypeID() const = 0;
    
    // Serialization support
    virtual void Serialize(void* outData) const {}
    virtual void Deserialize(const void* inData) {}
    
    // Owner access
    EntityObject& GetOwner() { return m_owner; }
    const EntityObject& GetOwner() const { return m_owner; }
    
protected:
    EntityObject& m_owner;
};

/**
 * Helper to generate unique type IDs for components at compile time
 */
template<typename T>
struct ComponentTypeID {
    static uint32_t Get() {
        static uint32_t id = s_nextID++;
        return id;
    }
private:
    static inline uint32_t s_nextID = 1; // Start at 1, 0 is reserved for "no component"
};

/**
 * Macro to simplify component type registration
 * Usage in component class:
 *   COMPONENT_TYPE(MyComponent)
 */
#define COMPONENT_TYPE(ClassName) \
    const char* GetTypeName() const override { return #ClassName; } \
    uint32_t GetTypeID() const override { return ComponentTypeID<ClassName>::Get(); }

} // namespace SecretEngine
