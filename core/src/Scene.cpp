// SecretEngine
// Module: core
// Responsibility: Scene implementation

#include <SecretEngine/Scene.h>
#include <algorithm>
#include <cstring>

namespace SecretEngine {

Scene::Scene(const std::string& name)
    : m_name(name)
    , m_isPlaying(false)
    , m_nextEntityID(1)
{
}

Scene::~Scene() {
    // Destroy all entities
    for (auto& entity : m_entities) {
        if (entity) {
            entity->OnDestroy();
        }
    }
    m_entities.clear();
    m_entityMap.clear();
    m_entitiesByTag.clear();
    m_fastAccess.Clear();
}

EntityObject* Scene::CreateEntity(const std::string& name) {
    Entity handle = { m_nextEntityID++, 1 };
    
    auto entity = std::make_unique<EntityObject>(handle, name);
    EntityObject* entityPtr = entity.get();
    
    // Set up component callbacks
    entityPtr->SetComponentAddedCallback(
        [this](IComponent& comp) { OnComponentAdded(comp); }
    );
    entityPtr->SetComponentRemovedCallback(
        [this](IComponent& comp) { OnComponentRemoved(comp); }
    );
    
    // Add to maps
    m_entityMap[handle.id] = entityPtr;
    
    // Add to tag map if tag is set
    if (!entityPtr->GetTag().empty()) {
        m_entitiesByTag[entityPtr->GetTag()].push_back(entityPtr);
    }
    
    // Store entity
    m_entities.push_back(std::move(entity));
    
    // Initialize if scene is playing
    if (m_isPlaying) {
        InitializeEntity(entityPtr);
    }
    
    return entityPtr;
}

bool Scene::DestroyEntity(EntityObject* entity) {
    if (!entity) {
        return false;
    }
    
    // Mark for destruction instead of immediate removal
    entity->MarkForDestruction();
    return true;
}

void Scene::CollectGarbage() {
    // Remove entities marked for destruction
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
            [this](const EntityPtr& entity) {
                if (entity->IsMarkedForDestruction()) {
                    // Remove from maps
                    m_entityMap.erase(entity->GetHandle().id);
                    
                    // Remove from tag map
                    const std::string& tag = entity->GetTag();
                    if (!tag.empty()) {
                        auto& taggedEntities = m_entitiesByTag[tag];
                        taggedEntities.erase(
                            std::remove(taggedEntities.begin(), taggedEntities.end(), entity.get()),
                            taggedEntities.end()
                        );
                    }
                    
                    // Call OnDestroy
                    entity->OnDestroy();
                    
                    return true;
                }
                return false;
            }),
        m_entities.end()
    );
}

EntityObject* Scene::FindEntityByName(const std::string& name) const {
    for (const auto& entity : m_entities) {
        if (entity->GetName() == name) {
            return entity.get();
        }
    }
    return nullptr;
}

EntityObject* Scene::FindEntityByTag(const std::string& tag) const {
    auto it = m_entitiesByTag.find(tag);
    if (it != m_entitiesByTag.end() && !it->second.empty()) {
        return it->second[0];
    }
    return nullptr;
}

EntityObject* Scene::FindEntityByHandle(Entity handle) const {
    auto it = m_entityMap.find(handle.id);
    return it != m_entityMap.end() ? it->second : nullptr;
}

std::vector<EntityObject*> Scene::FindEntitiesByTag(const std::string& tag) const {
    auto it = m_entitiesByTag.find(tag);
    if (it != m_entitiesByTag.end()) {
        return it->second;
    }
    return {};
}

void Scene::Play() {
    if (m_isPlaying) {
        return;
    }
    
    m_isPlaying = true;
    
    // Initialize all entities
    for (auto& entity : m_entities) {
        InitializeEntity(entity.get());
    }
}

void Scene::Stop() {
    if (!m_isPlaying) {
        return;
    }
    
    m_isPlaying = false;
    
    // Disable all entities
    for (auto& entity : m_entities) {
        if (entity->IsActive()) {
            entity->OnDisable();
        }
    }
}

void Scene::Update(float deltaTime) {
    if (!m_isPlaying) {
        return;
    }
    
    // Update all root entities (they will update their children)
    for (auto& entity : m_entities) {
        if (!entity->GetParent()) { // Only update root entities
            entity->OnUpdate(deltaTime);
        }
    }
    
    // Collect garbage after update
    CollectGarbage();
}

void Scene::FixedUpdate(float deltaTime) {
    if (!m_isPlaying) {
        return;
    }
    
    // Fixed update all root entities
    for (auto& entity : m_entities) {
        if (!entity->GetParent()) {
            entity->OnFixedUpdate(deltaTime);
        }
    }
}

void Scene::LateUpdate(float deltaTime) {
    if (!m_isPlaying) {
        return;
    }
    
    // Late update all root entities
    for (auto& entity : m_entities) {
        if (!entity->GetParent()) {
            entity->OnLateUpdate(deltaTime);
        }
    }
}

CameraComponentNew* Scene::FindMainCamera() const {
    for (auto* camera : m_fastAccess.cameras) {
        if (camera->IsMainCamera()) {
            return camera;
        }
    }
    
    // Return first camera if no main camera is set
    if (!m_fastAccess.cameras.empty()) {
        return m_fastAccess.cameras[0];
    }
    
    return nullptr;
}

bool Scene::SaveToFile(const std::string& filepath) {
    // TODO: Implement JSON serialization
    return false;
}

bool Scene::LoadFromFile(const std::string& filepath) {
    // TODO: Implement JSON deserialization
    return false;
}

void Scene::OnComponentAdded(IComponent& component) {
    RegisterComponentForFastAccess(component);
}

void Scene::OnComponentRemoved(IComponent& component) {
    UnregisterComponentFromFastAccess(component);
}

void Scene::RegisterComponentForFastAccess(IComponent& component) {
    // Register component in fast access lists based on type
    // Use static_cast since we know the type from GetTypeName()
    const char* typeName = component.GetTypeName();
    
    if (strcmp(typeName, "TransformComponentNew") == 0) {
        m_fastAccess.transforms.push_back(static_cast<TransformComponentNew*>(&component));
    }
    else if (strcmp(typeName, "MeshComponentNew") == 0) {
        m_fastAccess.meshes.push_back(static_cast<MeshComponentNew*>(&component));
    }
    else if (strcmp(typeName, "CameraComponentNew") == 0) {
        m_fastAccess.cameras.push_back(static_cast<CameraComponentNew*>(&component));
    }
    else if (strcmp(typeName, "LightComponentNew") == 0) {
        m_fastAccess.lights.push_back(static_cast<LightComponentNew*>(&component));
    }
}

void Scene::UnregisterComponentFromFastAccess(IComponent& component) {
    // Remove component from fast access lists
    const char* typeName = component.GetTypeName();
    
    if (strcmp(typeName, "TransformComponentNew") == 0) {
        auto* transform = static_cast<TransformComponentNew*>(&component);
        auto& vec = m_fastAccess.transforms;
        vec.erase(std::remove(vec.begin(), vec.end(), transform), vec.end());
    }
    else if (strcmp(typeName, "MeshComponentNew") == 0) {
        auto* mesh = static_cast<MeshComponentNew*>(&component);
        auto& vec = m_fastAccess.meshes;
        vec.erase(std::remove(vec.begin(), vec.end(), mesh), vec.end());
    }
    else if (strcmp(typeName, "CameraComponentNew") == 0) {
        auto* camera = static_cast<CameraComponentNew*>(&component);
        auto& vec = m_fastAccess.cameras;
        vec.erase(std::remove(vec.begin(), vec.end(), camera), vec.end());
    }
    else if (strcmp(typeName, "LightComponentNew") == 0) {
        auto* light = static_cast<LightComponentNew*>(&component);
        auto& vec = m_fastAccess.lights;
        vec.erase(std::remove(vec.begin(), vec.end(), light), vec.end());
    }
}

void Scene::InitializeEntity(EntityObject* entity) {
    if (!entity) {
        return;
    }
    
    entity->OnAwake();
    entity->OnStart();
}

} // namespace SecretEngine
