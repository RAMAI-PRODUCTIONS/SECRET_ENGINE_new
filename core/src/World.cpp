#include <SecretEngine/IWorld.h>
#include <SecretEngine/Entity.h>
#include <map>
#include <vector>
#include <algorithm>
#include <SecretEngine/Components.h>
#include <SecretEngine/ILogger.h>
#include <fstream>
#include <string.h>
#include <array>

namespace SecretEngine {
    // Performance-optimized ECS storage
    // Uses sparse arrays for O(1) component access instead of O(log n) map lookups
    static constexpr uint32_t MAX_ENTITIES = 8192;
    static constexpr uint32_t MAX_COMPONENT_TYPES = 32;
    
    class World : public IWorld {
    public:
        World() {
            // Pre-allocate component arrays for cache-friendly access
            for (uint32_t i = 0; i < MAX_COMPONENT_TYPES; ++i) {
                m_componentArrays[i].resize(MAX_ENTITIES, nullptr);
            }
        }
        
        Entity CreateEntity() override {
            Entity e = { m_nextID++, 1 };
            m_entities.push_back(e);
            
            // Grow component arrays if needed
            if (e.id >= m_componentArrays[0].size()) {
                for (uint32_t i = 0; i < MAX_COMPONENT_TYPES; ++i) {
                    m_componentArrays[i].resize(e.id + 1024, nullptr);
                }
            }
            
            return e;
        }

        void DestroyEntity(Entity e) override {
            auto it = std::find(m_entities.begin(), m_entities.end(), e);
            if (it != m_entities.end()) {
                m_entities.erase(it);
                
                // Clear components for this entity (O(1) per type instead of O(log n))
                if (e.id < m_componentArrays[0].size()) {
                    for (uint32_t i = 0; i < MAX_COMPONENT_TYPES; ++i) {
                        m_componentArrays[i][e.id] = nullptr;
                    }
                }
            }
        }

        void AddComponent(Entity e, uint32_t typeID, void* data) override {
            if (typeID >= MAX_COMPONENT_TYPES) return;
            if (e.id >= m_componentArrays[typeID].size()) {
                m_componentArrays[typeID].resize(e.id + 1024, nullptr);
            }
            m_componentArrays[typeID][e.id] = data;
        }

        void* GetComponent(Entity e, uint32_t typeID) override {
            // O(1) lookup instead of O(log n) - major performance win
            if (typeID >= MAX_COMPONENT_TYPES) return nullptr;
            if (e.id >= m_componentArrays[typeID].size()) return nullptr;
            return m_componentArrays[typeID][e.id];
        }

        const std::vector<Entity>& GetAllEntities() override {
            return m_entities;
        }

        void LoadScene(const char* path) override {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                return;
            }

            char magic[4];
            file.read(magic, 4);
            if (strncmp(magic, "SCN", 3) != 0) return;

            uint32_t count = 0;
            file.read((char*)&count, 4);

            for (uint32_t i = 0; i < count; i++) {
                auto e = CreateEntity();
                
                auto t = new TransformComponent();
                file.read((char*)t->position, 12);
                file.read((char*)t->rotation, 12);
                file.read((char*)t->scale, 12);
                AddComponent(e, TransformComponent::TypeID, t);

                auto m = new MeshComponent();
                strcpy(m->meshPath, "cube");
                file.read((char*)m->color, 12); m->color[3] = 1.0f;
                AddComponent(e, MeshComponent::TypeID, m);
            }
        }

    private:
        uint32_t m_nextID = 1;
        std::vector<Entity> m_entities;
        
        // Cache-friendly component storage: O(1) access instead of O(log n)
        // componentArrays[typeID][entityID] = component pointer
        std::array<std::vector<void*>, MAX_COMPONENT_TYPES> m_componentArrays;
    };

    // To be instantiated by Core
    IWorld* CreateWorld() {
        return new World();
    }
}
