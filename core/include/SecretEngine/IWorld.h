// SecretEngine
// Module: core
// Responsibility: Pure virtual interface for the game world
#pragma once
#include <SecretEngine/Entity.h>
#include <vector>

namespace SecretEngine {
    class IWorld {
    public:
        virtual ~IWorld() = default;
        
        virtual Entity CreateEntity() = 0;
        virtual void DestroyEntity(Entity e) = 0;
        
        virtual void AddComponent(Entity e, uint32_t typeID, void* data) = 0;
        virtual void* GetComponent(Entity e, uint32_t typeID) = 0;
        
        virtual const std::vector<Entity>& GetAllEntities() = 0;
        virtual void LoadScene(const char* path) = 0;
    };
}
