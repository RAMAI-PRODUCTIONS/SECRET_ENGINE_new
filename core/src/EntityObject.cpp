// SecretEngine
// Module: core
// Responsibility: EntityObject implementation

#include <SecretEngine/EntityObject.h>
#include <algorithm>

namespace SecretEngine {

EntityObject::EntityObject(Entity handle, const std::string& name)
    : m_handle(handle)
    , m_name(name)
    , m_tag("")
    , m_active(true)
    , m_awake(false)
    , m_started(false)
    , m_markedForDestruction(false)
    , m_parent(nullptr)
{
}

EntityObject::~EntityObject() {
    // Detach from parent
    if (m_parent) {
        m_parent->RemoveChild(this);
    }
    
    // Detach all children
    for (auto* child : m_children) {
        child->m_parent = nullptr;
    }
    m_children.clear();
    
    // Destroy all components
    for (auto& component : m_components) {
        if (IsActive()) {
            component->OnDisable();
        }
        component->OnDestroy();
    }
    m_components.clear();
    m_componentMap.clear();
}

void EntityObject::SetParent(EntityObject* parent) {
    if (m_parent == parent) {
        return;
    }
    
    // Detach from current parent
    if (m_parent) {
        m_parent->RemoveChild(this);
    }
    
    // Attach to new parent
    m_parent = parent;
    if (m_parent) {
        m_parent->AddChild(this);
    }
    
    // Update active state based on new hierarchy
    RecursiveActiveUpdate();
}

void EntityObject::DetachFromParent() {
    SetParent(nullptr);
}

bool EntityObject::IsDescendantOf(const EntityObject* entity) const {
    if (!entity) return false;
    
    const EntityObject* current = m_parent;
    while (current) {
        if (current == entity) {
            return true;
        }
        current = current->m_parent;
    }
    return false;
}

void EntityObject::SetActive(bool active) {
    if (m_active == active) {
        return;
    }
    
    m_active = active;
    
    // Propagate to children
    RecursiveActiveUpdate();
}

bool EntityObject::IsActive() const {
    if (!m_active) {
        return false;
    }
    
    // Check parent hierarchy
    const EntityObject* current = m_parent;
    while (current) {
        if (!current->m_active) {
            return false;
        }
        current = current->m_parent;
    }
    
    return true;
}

void EntityObject::OnAwake() {
    if (m_awake) {
        return;
    }
    
    m_awake = true;
    
    for (auto& component : m_components) {
        component->OnAwake();
    }
    
    // Awake children
    for (auto* child : m_children) {
        child->OnAwake();
    }
}

void EntityObject::OnStart() {
    if (m_started || !m_awake) {
        return;
    }
    
    m_started = true;
    
    if (IsActive()) {
        for (auto& component : m_components) {
            component->OnStart();
        }
    }
    
    // Start children
    for (auto* child : m_children) {
        child->OnStart();
    }
}

void EntityObject::OnUpdate(float deltaTime) {
    if (!IsActive()) {
        return;
    }
    
    for (auto& component : m_components) {
        component->OnUpdate(deltaTime);
    }
    
    // Update children
    for (auto* child : m_children) {
        child->OnUpdate(deltaTime);
    }
}

void EntityObject::OnFixedUpdate(float deltaTime) {
    if (!IsActive()) {
        return;
    }
    
    for (auto& component : m_components) {
        component->OnFixedUpdate(deltaTime);
    }
    
    // Fixed update children
    for (auto* child : m_children) {
        child->OnFixedUpdate(deltaTime);
    }
}

void EntityObject::OnLateUpdate(float deltaTime) {
    if (!IsActive()) {
        return;
    }
    
    for (auto& component : m_components) {
        component->OnLateUpdate(deltaTime);
    }
    
    // Late update children
    for (auto* child : m_children) {
        child->OnLateUpdate(deltaTime);
    }
}

void EntityObject::OnDisable() {
    if (!m_awake) {
        return;
    }
    
    for (auto& component : m_components) {
        component->OnDisable();
    }
    
    // Disable children
    for (auto* child : m_children) {
        if (child->IsSelfActive()) {
            child->OnDisable();
        }
    }
}

void EntityObject::OnDestroy() {
    if (!m_awake) {
        return;
    }
    
    for (auto& component : m_components) {
        component->OnDestroy();
    }
    
    // Destroy children
    for (auto* child : m_children) {
        child->OnDestroy();
    }
}

void EntityObject::RecursiveActiveUpdate() {
    bool wasActive = IsActive();
    bool isActive = IsActive();
    
    if (wasActive != isActive) {
        if (isActive && m_awake) {
            // Became active
            for (auto& component : m_components) {
                component->OnEnable();
            }
        } else if (!isActive && m_awake) {
            // Became inactive
            for (auto& component : m_components) {
                component->OnDisable();
            }
        }
    }
    
    // Propagate to children
    for (auto* child : m_children) {
        child->RecursiveActiveUpdate();
    }
}

void EntityObject::AddChild(EntityObject* child) {
    if (!child || child == this) {
        return;
    }
    
    // Check if already a child
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        return;
    }
    
    m_children.push_back(child);
}

void EntityObject::RemoveChild(EntityObject* child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it);
    }
}

} // namespace SecretEngine
