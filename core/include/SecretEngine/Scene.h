// SecretEngine
// Module: core
// Responsibility: Scene management with entity lifecycle and fast component access
// Inspired by: Overload Engine's Scene pattern

#pragma once
#include <SecretEngine/EntityObject.h>
#include <SecretEngine/ComponentsNew.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace SecretEngine {

/**
 * Scene manages a collection of entities with lifecycle and fast component access.
 * Provides optimized access to frequently-used component types for rendering and systems.
 */
class Scene {
public:
    using EntityPtr = std::unique_ptr<EntityObject>;
    
    /**
     * Fast access to specific component types for rendering/systems
     * Avoids iterating through all entities to find specific components
     */
    struct FastAccess {
        std::vector<MeshComponentNew*> meshes;
        std::vector<CameraComponentNew*> cameras;
        std::vector<LightComponentNew*> lights;
        std::vector<TransformComponentNew*> transforms;
        
        void Clear() {
            meshes.clear();
            cameras.clear();
            lights.clear();
            transforms.clear();
        }
    };
    
    explicit Scene(const std::string& name = "Untitled Scene");
    ~Scene();
    
    // Entity Management
    EntityObject* CreateEntity(const std::string& name = "Entity");
    bool DestroyEntity(EntityObject* entity);
    void CollectGarbage(); // Remove entities marked for destruction
    
    // Entity Queries
    EntityObject* FindEntityByName(const std::string& name) const;
    EntityObject* FindEntityByTag(const std::string& tag) const;
    EntityObject* FindEntityByHandle(Entity handle) const;
    std::vector<EntityObject*> FindEntitiesByTag(const std::string& tag) const;
    
    const std::vector<EntityPtr>& GetAllEntities() const { return m_entities; }
    
    // Lifecycle
    void Play();
    void Stop();
    bool IsPlaying() const { return m_isPlaying; }
    
    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);
    void LateUpdate(float deltaTime);
    
    // Fast Access
    const FastAccess& GetFastAccess() const { return m_fastAccess; }
    
    // Camera helpers
    CameraComponentNew* FindMainCamera() const;
    
    // Properties
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    
    // Serialization
    bool SaveToFile(const std::string& filepath);
    bool LoadFromFile(const std::string& filepath);
    
private:
    void OnComponentAdded(IComponent& component);
    void OnComponentRemoved(IComponent& component);
    
    void RegisterComponentForFastAccess(IComponent& component);
    void UnregisterComponentFromFastAccess(IComponent& component);
    
    void InitializeEntity(EntityObject* entity);
    
    std::string m_name;
    std::vector<EntityPtr> m_entities;
    std::unordered_map<uint32_t, EntityObject*> m_entityMap; // Handle ID -> Entity
    std::unordered_map<std::string, std::vector<EntityObject*>> m_entitiesByTag;
    
    FastAccess m_fastAccess;
    bool m_isPlaying = false;
    uint32_t m_nextEntityID = 1;
};

} // namespace SecretEngine
