#include <SecretEngine/IWorld.h>
#include <SecretEngine/Entity.h>
#include <map>
#include <vector>
#include <algorithm>
#include <SecretEngine/Components.h>
#include <SecretEngine/ILogger.h>
#include <fstream>
#include <string.h>

namespace SecretEngine {
    class World : public IWorld {
    public:
        Entity CreateEntity() override {
            Entity e = { m_nextID++, 1 };
            m_entities.push_back(e);
            return e;
        }

        void DestroyEntity(Entity e) override {
            auto it = std::find(m_entities.begin(), m_entities.end(), e);
            if (it != m_entities.end()) {
                m_entities.erase(it);
                for (auto& pair : m_components) {
                    pair.second.erase(e.id);
                }
            }
        }

        void AddComponent(Entity e, uint32_t typeID, void* data) override {
            m_components[typeID][e.id] = data;
        }

        void* GetComponent(Entity e, uint32_t typeID) override {
            if (m_components.count(typeID) && m_components[typeID].count(e.id)) {
                return m_components[typeID][e.id];
            }
            return nullptr;
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
        std::map<uint32_t, std::map<uint32_t, void*>> m_components;
    };

    // To be instantiated by Core
    IWorld* CreateWorld() {
        return new World();
    }
}
