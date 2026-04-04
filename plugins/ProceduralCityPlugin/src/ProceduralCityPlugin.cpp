// SecretEngine - Procedural City Generator
// Generates cyberpunk-style buildings with neon lights

#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/Components.h>
#include <random>
#include <cmath>

namespace SecretEngine {

struct BuildingSpec {
    float x, z;
    float width, height, depth;
    float hue;
    bool isTall;
};

class ProceduralCityPlugin : public IPlugin {
public:
    const char* GetName() const override { return "ProceduralCityPlugin"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override {
        m_core = core;
        m_logger = core->GetLogger();
        if (m_logger) m_logger->LogInfo("ProceduralCity", "🏙️ Procedural City Plugin Loaded");
    }

    void OnActivate() override {
        if (m_logger) m_logger->LogInfo("ProceduralCity", "🏗️ Generating Cyberpunk City...");
        GenerateCity();
    }

    void OnDeactivate() override {}
    void OnUnload() override {}
    void* GetInterface(uint32_t id) override { return nullptr; }
    void OnUpdate(float dt) override {
        // Animate neon lights
        m_time += dt;
    }

private:
    void GenerateCity() {
        IWorld* world = m_core->GetWorld();
        if (!world) return;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> randFloat(0.0f, 1.0f);

        // Generate ground plane
        CreateGround(world);

        // Generate city grid
        const float spacing = 6.5f;
        const float radius = 42.0f;
        
        for (float i = -radius; i <= radius; i += spacing) {
            for (float j = -radius; j <= radius; j += spacing) {
                // Leave central area clear
                if (std::abs(i) < 7.0f && std::abs(j) < 7.0f) continue;
                
                // Random gaps
                if (randFloat(gen) > 0.92f) continue;
                
                float x = i + (randFloat(gen) - 0.5f) * 1.2f;
                float z = j + (randFloat(gen) - 0.5f) * 1.2f;
                
                CreateBuilding(world, x, z, false, gen);
            }
        }

        // Landmark towers
        float landmarks[][2] = {
            {-18, -15}, {22, -12}, {-20, 18}, {15, 24},
            {-25, 5}, {28, -8}, {5, -28}, {-5, 30}
        };
        
        for (auto& pos : landmarks) {
            CreateBuilding(world, pos[0], pos[1], true, gen);
        }

        if (m_logger) m_logger->LogInfo("ProceduralCity", "✅ City generation complete!");
    }

    void CreateGround(IWorld* world) {
        Entity ground = world->CreateEntity();
        
        TransformComponent transform;
        transform.position[0] = 0.0f;
        transform.position[1] = 0.0f;
        transform.position[2] = -0.05f;
        transform.scale[0] = 200.0f;
        transform.scale[1] = 200.0f;
        transform.scale[2] = 1.0f;
        world->AddComponent(ground, TransformComponent::TypeID, &transform);
        
        MeshComponent mesh;
        strncpy(mesh.meshPath, "meshes/cube.meshbin", sizeof(mesh.meshPath));
        // Dark metallic ground
        mesh.color[0] = 0.04f;
        mesh.color[1] = 0.04f;
        mesh.color[2] = 0.1f;
        mesh.color[3] = 1.0f;
        world->AddComponent(ground, MeshComponent::TypeID, &mesh);
    }

    void CreateBuilding(IWorld* world, float x, float z, bool isTall, std::mt19937& gen) {
        std::uniform_real_distribution<float> randFloat(0.0f, 1.0f);
        
        float height = isTall ? (28.0f + randFloat(gen) * 12.0f) : (8.0f + randFloat(gen) * 22.0f);
        float width = isTall ? 4.2f : (2.8f + randFloat(gen) * 3.5f);
        float depth = isTall ? 4.2f : (2.8f + randFloat(gen) * 3.5f);
        
        // Main building
        Entity building = world->CreateEntity();
        
        TransformComponent transform;
        transform.position[0] = x;
        transform.position[1] = z;
        transform.position[2] = height / 2.0f;
        transform.scale[0] = width;
        transform.scale[1] = depth;
        transform.scale[2] = height;
        world->AddComponent(building, TransformComponent::TypeID, &transform);
        
        MeshComponent mesh;
        strncpy(mesh.meshPath, "meshes/cube.meshbin", sizeof(mesh.meshPath));
        // Dark building color
        mesh.color[0] = 0.08f + randFloat(gen) * 0.05f;
        mesh.color[1] = 0.12f + randFloat(gen) * 0.05f;
        mesh.color[2] = 0.17f + randFloat(gen) * 0.05f;
        mesh.color[3] = 1.0f;
        world->AddComponent(building, MeshComponent::TypeID, &mesh);
        
        // Add neon window strips
        int windowRows = static_cast<int>(height / 1.2f);
        for (int row = 0; row < windowRows; row++) {
            float yPos = -height/2.0f + 0.6f + row * 1.2f;
            if (yPos > height/2.0f - 0.8f) continue;
            
            // Front windows
            CreateNeonStrip(world, x, z, yPos + height/2.0f, width - 0.6f, 0.25f, depth/2.0f + 0.02f, true, gen);
            // Back windows
            CreateNeonStrip(world, x, z, yPos + height/2.0f, width - 0.6f, 0.25f, -depth/2.0f - 0.02f, true, gen);
        }
        
        // Rooftop beacon
        CreateBeacon(world, x, z, height + 0.4f, gen);
        
        // Billboard
        if (randFloat(gen) > 0.5f) {
            CreateBillboard(world, x + (randFloat(gen) > 0.5f ? 1 : -1) * (width/2.0f + 0.15f), 
                          z, height * 0.6f, gen);
        }
    }

    void CreateNeonStrip(IWorld* world, float x, float z, float y, 
                        float width, float height, float offset, bool horizontal, std::mt19937& gen) {
        std::uniform_real_distribution<float> randFloat(0.0f, 1.0f);
        
        Entity strip = world->CreateEntity();
        
        TransformComponent transform;
        transform.position[0] = x + (horizontal ? 0 : offset);
        transform.position[1] = z + (horizontal ? offset : 0);
        transform.position[2] = y;
        transform.scale[0] = horizontal ? width : 0.05f;
        transform.scale[1] = horizontal ? 0.05f : width;
        transform.scale[2] = height;
        world->AddComponent(strip, TransformComponent::TypeID, &transform);
        
        MeshComponent mesh;
        strncpy(mesh.meshPath, "meshes/cube.meshbin", sizeof(mesh.meshPath));
        // Bright neon orange/yellow
        mesh.color[0] = 1.0f;
        mesh.color[1] = 0.67f + randFloat(gen) * 0.2f;
        mesh.color[2] = 0.4f;
        mesh.color[3] = 1.0f;
        world->AddComponent(strip, MeshComponent::TypeID, &mesh);
    }

    void CreateBeacon(IWorld* world, float x, float z, float y, std::mt19937& gen) {
        Entity beacon = world->CreateEntity();
        
        TransformComponent transform;
        transform.position[0] = x;
        transform.position[1] = z;
        transform.position[2] = y;
        transform.scale[0] = 0.4f;
        transform.scale[1] = 0.4f;
        transform.scale[2] = 0.8f;
        world->AddComponent(beacon, TransformComponent::TypeID, &transform);
        
        MeshComponent mesh;
        strncpy(mesh.meshPath, "meshes/cube.meshbin", sizeof(mesh.meshPath));
        // Bright pink/magenta
        mesh.color[0] = 1.0f;
        mesh.color[1] = 0.2f;
        mesh.color[2] = 0.4f;
        mesh.color[3] = 1.0f;
        world->AddComponent(beacon, MeshComponent::TypeID, &mesh);
    }

    void CreateBillboard(IWorld* world, float x, float z, float y, std::mt19937& gen) {
        std::uniform_int_distribution<int> colorChoice(0, 3);
        
        Entity billboard = world->CreateEntity();
        
        TransformComponent transform;
        transform.position[0] = x;
        transform.position[1] = z;
        transform.position[2] = y;
        transform.scale[0] = 2.2f;
        transform.scale[1] = 0.1f;
        transform.scale[2] = 1.2f;
        world->AddComponent(billboard, TransformComponent::TypeID, &transform);
        
        MeshComponent mesh;
        strncpy(mesh.meshPath, "meshes/cube.meshbin", sizeof(mesh.meshPath));
        
        // Random neon colors
        float colors[][3] = {
            {1.0f, 0.0f, 0.67f},  // Pink
            {0.0f, 1.0f, 1.0f},   // Cyan
            {1.0f, 0.4f, 0.0f},   // Orange
            {0.67f, 0.27f, 1.0f}  // Purple
        };
        int choice = colorChoice(gen);
        mesh.color[0] = colors[choice][0];
        mesh.color[1] = colors[choice][1];
        mesh.color[2] = colors[choice][2];
        mesh.color[3] = 0.9f;
        world->AddComponent(billboard, MeshComponent::TypeID, &mesh);
    }

    ICore* m_core = nullptr;
    ILogger* m_logger = nullptr;
    float m_time = 0.0f;
};

extern "C" IPlugin* CreateProceduralCityPlugin() {
    return new ProceduralCityPlugin();
}

} // namespace SecretEngine
