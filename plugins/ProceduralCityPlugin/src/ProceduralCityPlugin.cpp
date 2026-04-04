// SecretEngine - Procedural City Generator
// Generates cyberpunk-style buildings with neon lights

#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/IRenderer.h>
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
        
        // Register as a capability so OnActivate gets called
        core->RegisterCapability("procedural_city", this);
    }

    void OnActivate() override {
        if (m_logger) m_logger->LogInfo("ProceduralCity", "🏗️ Generating Cyberpunk City...");
        
        // Get renderer to submit instances
        m_renderer = reinterpret_cast<IRenderer*>(m_core->GetCapability("rendering"));
        if (!m_renderer) {
            if (m_logger) m_logger->LogError("ProceduralCity", "Renderer not available!");
            return;
        }
        
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
        // Submit ground plane directly to renderer
        // Mesh slot 0 is typically the cube mesh
        uint32_t meshSlot = 0;
        
        // Ground at z=-0.05, scaled 200x200
        m_renderer->AddInstance(meshSlot, 
                               0.0f, 0.0f, -0.05f,  // position
                               200.0f, 200.0f, 1.0f,  // scale
                               0, // textureID
                               0.04f, 0.04f, 0.1f, 1.0f); // dark metallic color
    }

    void CreateBuilding(IWorld* world, float x, float z, bool isTall, std::mt19937& gen) {
        std::uniform_real_distribution<float> randFloat(0.0f, 1.0f);
        
        float height = isTall ? (28.0f + randFloat(gen) * 12.0f) : (8.0f + randFloat(gen) * 22.0f);
        float width = isTall ? 4.2f : (2.8f + randFloat(gen) * 3.5f);
        float depth = isTall ? 4.2f : (2.8f + randFloat(gen) * 3.5f);
        
        uint32_t meshSlot = 0; // cube mesh
        
        // Main building
        m_renderer->AddInstance(meshSlot,
                               x, z, height / 2.0f,  // position
                               width, depth, height,  // scale
                               0,  // textureID
                               0.08f + randFloat(gen) * 0.05f,  // dark building color
                               0.12f + randFloat(gen) * 0.05f,
                               0.17f + randFloat(gen) * 0.05f,
                               1.0f);
        
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
        
        uint32_t meshSlot = 0;
        
        m_renderer->AddInstance(meshSlot,
                               x + (horizontal ? 0 : offset),
                               z + (horizontal ? offset : 0),
                               y,
                               horizontal ? width : 0.05f,
                               horizontal ? 0.05f : width,
                               height,
                               0,  // textureID
                               1.0f,  // bright neon orange/yellow
                               0.67f + randFloat(gen) * 0.2f,
                               0.4f,
                               1.0f);
    }

    void CreateBeacon(IWorld* world, float x, float z, float y, std::mt19937& gen) {
        uint32_t meshSlot = 0;
        
        m_renderer->AddInstance(meshSlot,
                               x, z, y,
                               0.4f, 0.4f, 0.8f,  // scale
                               0,  // textureID
                               1.0f, 0.2f, 0.4f, 1.0f);  // bright pink/magenta
    }

    void CreateBillboard(IWorld* world, float x, float z, float y, std::mt19937& gen) {
        std::uniform_int_distribution<int> colorChoice(0, 3);
        
        uint32_t meshSlot = 0;
        
        // Random neon colors
        float colors[][3] = {
            {1.0f, 0.0f, 0.67f},  // Pink
            {0.0f, 1.0f, 1.0f},   // Cyan
            {1.0f, 0.4f, 0.0f},   // Orange
            {0.67f, 0.27f, 1.0f}  // Purple
        };
        int choice = colorChoice(gen);
        
        m_renderer->AddInstance(meshSlot,
                               x, z, y,
                               2.2f, 0.1f, 1.2f,  // scale
                               0,  // textureID
                               colors[choice][0],
                               colors[choice][1],
                               colors[choice][2],
                               0.9f);
    }

    ICore* m_core = nullptr;
    ILogger* m_logger = nullptr;
    IRenderer* m_renderer = nullptr;
    float m_time = 0.0f;
};

extern "C" IPlugin* CreateProceduralCityPlugin() {
    return new ProceduralCityPlugin();
}

} // namespace SecretEngine
