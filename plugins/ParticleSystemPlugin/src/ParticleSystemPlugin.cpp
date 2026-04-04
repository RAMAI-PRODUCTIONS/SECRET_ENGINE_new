// SecretEngine - Particle System
// Rain, sparks, dust for cyberpunk atmosphere

#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/Components.h>
#include <random>
#include <vector>
#include <cmath>

namespace SecretEngine {

struct Particle {
    float pos[3];
    float velocity[3];
    float lifetime;
    float maxLifetime;
    uint8_t type; // 0=rain, 1=spark, 2=dust
};

class ParticleSystemPlugin : public IPlugin {
public:
    const char* GetName() const override { return "ParticleSystemPlugin"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override {
        m_core = core;
        m_logger = core->GetLogger();
        if (m_logger) m_logger->LogInfo("ParticleSystem", "💧 Particle System Loaded");
        
        // Register as a capability so OnActivate gets called
        core->RegisterCapability("particle_system", this);
    }

    void OnActivate() override {
        if (m_logger) m_logger->LogInfo("ParticleSystem", "🌧️ Initializing particles...");
        InitializeParticles();
    }

    void OnDeactivate() override {
        m_particles.clear();
    }

    void OnUnload() override {}
    void* GetInterface(uint32_t id) override { return nullptr; }
    
    void OnUpdate(float dt) override {
        UpdateParticles(dt);
        RenderParticles();
    }

private:
    void InitializeParticles() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> randPos(-90.0f, 90.0f);
        std::uniform_real_distribution<float> randHeight(0.0f, 65.0f);
        std::uniform_real_distribution<float> randVel(0.6f, 1.4f);

        // Rain particles (8000 for performance)
        for (int i = 0; i < 8000; i++) {
            Particle p;
            p.pos[0] = randPos(gen);
            p.pos[1] = randPos(gen);
            p.pos[2] = randHeight(gen);
            p.velocity[0] = 0.0f;
            p.velocity[1] = 0.0f;
            p.velocity[2] = -randVel(gen) * 18.0f;
            p.lifetime = 100.0f;
            p.maxLifetime = 100.0f;
            p.type = 0; // rain
            m_particles.push_back(p);
        }

        // Sparks (600)
        std::uniform_real_distribution<float> sparkHeight(0.0f, 15.0f);
        for (int i = 0; i < 600; i++) {
            Particle p;
            p.pos[0] = randPos(gen);
            p.pos[1] = randPos(gen);
            p.pos[2] = sparkHeight(gen);
            p.velocity[0] = 0.0f;
            p.velocity[1] = 0.0f;
            p.velocity[2] = -1.2f;
            p.lifetime = 100.0f;
            p.maxLifetime = 100.0f;
            p.type = 1; // spark
            m_particles.push_back(p);
        }

        // Dust (2000)
        std::uniform_real_distribution<float> dustHeight(0.0f, 25.0f);
        for (int i = 0; i < 2000; i++) {
            Particle p;
            p.pos[0] = randPos(gen);
            p.pos[1] = randPos(gen);
            p.pos[2] = dustHeight(gen);
            p.velocity[0] = 0.0f;
            p.velocity[1] = 0.0f;
            p.velocity[2] = 0.4f;
            p.lifetime = 100.0f;
            p.maxLifetime = 100.0f;
            p.type = 2; // dust
            m_particles.push_back(p);
        }

        if (m_logger) {
            char msg[128];
            snprintf(msg, sizeof(msg), "✅ Created %zu particles", m_particles.size());
            m_logger->LogInfo("ParticleSystem", msg);
        }
    }

    void UpdateParticles(float dt) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> randPos(-85.0f, 85.0f);

        for (auto& p : m_particles) {
            // Update position
            p.pos[0] += p.velocity[0] * dt;
            p.pos[1] += p.velocity[1] * dt;
            p.pos[2] += p.velocity[2] * dt;

            // Reset based on type
            if (p.type == 0) { // Rain
                if (p.pos[2] < 0.0f) {
                    p.pos[2] = 55.0f + randPos(gen) * 0.2f;
                    p.pos[0] = randPos(gen);
                    p.pos[1] = randPos(gen);
                }
            } else if (p.type == 1) { // Sparks
                if (p.pos[2] < 0.0f) {
                    p.pos[2] = 12.0f;
                    p.pos[0] = randPos(gen);
                    p.pos[1] = randPos(gen);
                }
            } else if (p.type == 2) { // Dust
                if (p.pos[2] > 24.0f) {
                    p.pos[2] = 0.0f;
                    p.pos[0] = randPos(gen);
                    p.pos[1] = randPos(gen);
                }
            }
        }
    }

    void RenderParticles() {
        IWorld* world = m_core->GetWorld();
        if (!world) return;

        // Create/update particle entities (simplified - in real impl would batch)
        // For now, we'll create a few representative particles as visual entities
        static bool particleEntitiesCreated = false;
        if (!particleEntitiesCreated) {
            particleEntitiesCreated = true;
            
            // Create a few visible rain drops
            for (int i = 0; i < 50; i++) {
                if (i < m_particles.size()) {
                    Entity e = world->CreateEntity();
                    
                    TransformComponent transform;
                    transform.position[0] = m_particles[i].pos[0];
                    transform.position[1] = m_particles[i].pos[1];
                    transform.position[2] = m_particles[i].pos[2];
                    transform.scale[0] = 0.05f;
                    transform.scale[1] = 0.05f;
                    transform.scale[2] = 0.3f;
                    world->AddComponent(e, TransformComponent::TypeID, &transform);
                    
                    MeshComponent mesh;
                    strncpy(mesh.meshPath, "meshes/cube.meshbin", sizeof(mesh.meshPath));
                    mesh.color[0] = 0.53f;
                    mesh.color[1] = 0.73f;
                    mesh.color[2] = 1.0f;
                    mesh.color[3] = 0.7f;
                    world->AddComponent(e, MeshComponent::TypeID, &mesh);
                    
                    m_particleEntities.push_back(e);
                }
            }
        } else {
            // Update positions
            for (size_t i = 0; i < m_particleEntities.size() && i < m_particles.size(); i++) {
                auto* transform = static_cast<TransformComponent*>(
                    world->GetComponent(m_particleEntities[i], TransformComponent::TypeID)
                );
                if (transform) {
                    transform->position[0] = m_particles[i].pos[0];
                    transform->position[1] = m_particles[i].pos[1];
                    transform->position[2] = m_particles[i].pos[2];
                }
            }
        }
    }

    ICore* m_core = nullptr;
    ILogger* m_logger = nullptr;
    std::vector<Particle> m_particles;
    std::vector<Entity> m_particleEntities;
};

extern "C" IPlugin* CreateParticleSystemPlugin() {
    return new ParticleSystemPlugin();
}

} // namespace SecretEngine
