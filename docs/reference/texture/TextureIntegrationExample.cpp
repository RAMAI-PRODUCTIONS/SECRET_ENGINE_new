// ============================================================================
// COMPLETE TEXTURE INTEGRATION EXAMPLE
// Shows how to integrate textures into your existing renderer
// ============================================================================

#include "RendererPlugin.h"
#include "TextureManager.h"
#include "MegaGeometryRendererTextured.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>

using namespace SecretEngine;
using namespace SecretEngine::Textures;
using namespace SecretEngine::MegaGeometry;

// ============================================================================
// EXAMPLE 1: BASIC TEXTURED CUBE
// ============================================================================

void Example1_BasicTexturedCube(ICore* core, 
                                TextureManager* texMgr,
                                MegaGeometryRendererTextured* renderer) {
    
    core->GetLogger()->LogInfo("Example", "--- Example 1: Basic Textured Cube ---");
    
    // Step 1: Load texture (auto-converts PNG to ASTC)
    uint32_t brickTexture = texMgr->LoadTexture("textures/brick.png");
    
    if (brickTexture == UINT32_MAX) {
        core->GetLogger()->LogError("Example", "Failed to load brick texture!");
        return;
    }
    
    // Step 2: Load cube mesh
    renderer->LoadMesh("meshes/cube.mesh", 0);
    
    // Step 3: Create textured instance
    uint32_t cubeInstance = renderer->AddInstance(
        0,              // Mesh slot
        0.0f, 0.0f, 0.0f,  // Position
        brickTexture    // Texture ID
    );
    
    // Step 4: Set color tint (multiplied with texture)
    renderer->UpdateInstanceColor(cubeInstance, 1.0f, 1.0f, 1.0f, 1.0f);
    
    core->GetLogger()->LogInfo("Example", "✓ Created textured cube");
}

// ============================================================================
// EXAMPLE 2: MULTIPLE INSTANCES WITH DIFFERENT TEXTURES
// ============================================================================

void Example2_MultipleTextures(ICore* core, 
                               TextureManager* texMgr,
                               MegaGeometryRendererTextured* renderer) {
    
    core->GetLogger()->LogInfo("Example", "--- Example 2: Multiple Textures ---");
    
    // Load multiple textures
    struct TextureSet {
        const char* name;
        uint32_t id;
    };
    
    TextureSet textures[] = {
        {"textures/brick.png", UINT32_MAX},
        {"textures/wood.png", UINT32_MAX},
        {"textures/metal.png", UINT32_MAX},
        {"textures/grass.png", UINT32_MAX}
    };
    
    for (int i = 0; i < 4; ++i) {
        textures[i].id = texMgr->LoadTexture(textures[i].name);
        if (textures[i].id != UINT32_MAX) {
            core->GetLogger()->LogInfo("Example", 
                (std::string("✓ Loaded: ") + textures[i].name).c_str());
        }
    }
    
    // Create grid of textured cubes
    renderer->LoadMesh("meshes/cube.mesh", 0);
    
    for (int x = 0; x < 4; ++x) {
        for (int z = 0; z < 4; ++z) {
            uint32_t texIdx = (x + z) % 4;
            
            uint32_t instance = renderer->AddInstance(
                0,
                x * 3.0f, 0.0f, z * 3.0f,
                textures[texIdx].id
            );
            
            // Slight color variation
            float tint = 0.8f + (x + z) * 0.05f;
            renderer->UpdateInstanceColor(instance, tint, tint, tint, 1.0f);
        }
    }
    
    core->GetLogger()->LogInfo("Example", "✓ Created 16 textured cubes");
}

// ============================================================================
// EXAMPLE 3: DYNAMIC TEXTURE SWAPPING
// ============================================================================

void Example3_DynamicTextureSwap(ICore* core, 
                                 TextureManager* texMgr,
                                 MegaGeometryRendererTextured* renderer,
                                 float deltaTime) {
    
    static float timer = 0.0f;
    static uint32_t instance = UINT32_MAX;
    static uint32_t textures[3] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
    
    // Initialize once
    if (instance == UINT32_MAX) {
        core->GetLogger()->LogInfo("Example", "--- Example 3: Dynamic Texture Swap ---");
        
        textures[0] = texMgr->LoadTexture("textures/frame1.png");
        textures[1] = texMgr->LoadTexture("textures/frame2.png");
        textures[2] = texMgr->LoadTexture("textures/frame3.png");
        
        renderer->LoadMesh("meshes/billboard.mesh", 1);
        instance = renderer->AddInstance(1, 0.0f, 2.0f, 0.0f, textures[0]);
    }
    
    // Swap texture every 0.5 seconds (animated billboard)
    timer += deltaTime;
    if (timer > 0.5f) {
        timer = 0.0f;
        static int frame = 0;
        frame = (frame + 1) % 3;
        renderer->UpdateInstanceTexture(instance, textures[frame]);
    }
}

// ============================================================================
// EXAMPLE 4: TEXTURE ATLAS FOR SPRITES
// ============================================================================

void Example4_SpriteAtlas(ICore* core, 
                         TextureManager* texMgr,
                         MegaGeometryRendererTextured* renderer) {
    
    core->GetLogger()->LogInfo("Example", "--- Example 4: Sprite Atlas ---");
    
    // Define sprite textures to pack
    TextureManager::AtlasTextureInfo sprites[] = {
        {"sprites/player_idle.png", 0},
        {"sprites/player_walk1.png", 0},
        {"sprites/player_walk2.png", 0},
        {"sprites/player_jump.png", 0},
        {"sprites/enemy1.png", 0},
        {"sprites/enemy2.png", 0},
        {"sprites/coin.png", 0},
        {"sprites/powerup.png", 0}
    };
    
    // Create 2048x2048 atlas
    uint32_t atlasID = texMgr->CreateAtlas(sprites, 8, 2048);
    
    if (atlasID != UINT32_MAX) {
        core->GetLogger()->LogInfo("Example", "✓ Created sprite atlas");
        
        // Create sprite instances using atlas
        renderer->LoadMesh("meshes/quad.mesh", 2);
        
        for (int i = 0; i < 8; ++i) {
            uint32_t spriteInstance = renderer->AddInstance(
                2,
                i * 1.5f, 0.0f, 0.0f,
                sprites[i].slotIndex  // Use atlas slot
            );
            
            core->GetLogger()->LogInfo("Example", 
                (std::string("✓ Sprite ") + std::to_string(i) + " at slot " + 
                 std::to_string(sprites[i].slotIndex)).c_str());
        }
    }
}

// ============================================================================
// EXAMPLE 5: MATERIAL SYSTEM (PBR)
// ============================================================================

void Example5_PBRMaterial(ICore* core, 
                         TextureManager* texMgr,
                         MegaGeometryRendererTextured* renderer) {
    
    core->GetLogger()->LogInfo("Example", "--- Example 5: PBR Materials ---");
    
    // Create wood material
    TextureManager::Material woodMat = {};
    woodMat.albedoTexture = texMgr->LoadTexture("materials/wood_albedo.png");
    woodMat.normalTexture = texMgr->LoadTexture("materials/wood_normal.png");
    woodMat.metallicRoughnessTexture = texMgr->LoadTexture("materials/wood_mr.png");
    woodMat.albedoFactor[0] = 1.0f;
    woodMat.albedoFactor[1] = 1.0f;
    woodMat.albedoFactor[2] = 1.0f;
    woodMat.albedoFactor[3] = 1.0f;
    woodMat.metallicFactor = 0.1f;
    woodMat.roughnessFactor = 0.9f;
    
    uint32_t woodMatID = texMgr->CreateMaterial(woodMat);
    
    // Create metal material
    TextureManager::Material metalMat = {};
    metalMat.albedoTexture = texMgr->LoadTexture("materials/metal_albedo.png");
    metalMat.normalTexture = texMgr->LoadTexture("materials/metal_normal.png");
    metalMat.metallicRoughnessTexture = texMgr->LoadTexture("materials/metal_mr.png");
    metalMat.albedoFactor[0] = 0.8f;
    metalMat.albedoFactor[1] = 0.8f;
    metalMat.albedoFactor[2] = 0.8f;
    metalMat.albedoFactor[3] = 1.0f;
    metalMat.metallicFactor = 0.9f;
    metalMat.roughnessFactor = 0.2f;
    
    uint32_t metalMatID = texMgr->CreateMaterial(metalMat);
    
    core->GetLogger()->LogInfo("Example", 
        (std::string("✓ Created materials: Wood(") + std::to_string(woodMatID) + 
         "), Metal(" + std::to_string(metalMatID) + ")").c_str());
}

// ============================================================================
// EXAMPLE 6: BATCH TEXTURE UPDATES
// ============================================================================

void Example6_BatchUpdates(ICore* core, 
                          TextureManager* texMgr,
                          MegaGeometryRendererTextured* renderer) {
    
    core->GetLogger()->LogInfo("Example", "--- Example 6: Batch Updates ---");
    
    uint32_t texture1 = texMgr->LoadTexture("textures/pattern1.png");
    uint32_t texture2 = texMgr->LoadTexture("textures/pattern2.png");
    
    renderer->LoadMesh("meshes/cube.mesh", 0);
    
    // Create 1000 instances
    std::vector<uint32_t> instances;
    instances.reserve(1000);
    
    renderer->BeginBatchUpdate();  // Optimize for batch operations
    
    for (int i = 0; i < 1000; ++i) {
        float x = (i % 10) * 2.0f;
        float y = ((i / 10) % 10) * 2.0f;
        float z = (i / 100) * 2.0f;
        
        uint32_t tex = (i % 2 == 0) ? texture1 : texture2;
        uint32_t inst = renderer->AddInstance(0, x, y, z, tex);
        instances.push_back(inst);
    }
    
    renderer->EndBatchUpdate();
    
    core->GetLogger()->LogInfo("Example", "✓ Created 1000 textured instances");
}

// ============================================================================
// EXAMPLE 7: TEXTURE STATS AND DEBUGGING
// ============================================================================

void Example7_TextureStats(ICore* core, 
                          TextureManager* texMgr,
                          MegaGeometryRendererTextured* renderer) {
    
    core->GetLogger()->LogInfo("Example", "--- Example 7: Texture Stats ---");
    
    // Get texture VRAM usage
    uint64_t vram = texMgr->GetVRAMUsage();
    float vramMB = vram / (1024.0f * 1024.0f);
    
    // Get texture count
    uint32_t textureCount = texMgr->GetTextureCount();
    
    // Get render stats
    auto renderStats = renderer->GetStats();
    
    core->GetLogger()->LogInfo("Example", 
        (std::string("Texture VRAM: ") + std::to_string(vramMB) + " MB").c_str());
    core->GetLogger()->LogInfo("Example", 
        (std::string("Texture Count: ") + std::to_string(textureCount)).c_str());
    core->GetLogger()->LogInfo("Example", 
        (std::string("Instances: ") + std::to_string(renderStats.totalInstances)).c_str());
    core->GetLogger()->LogInfo("Example", 
        (std::string("Triangles: ") + std::to_string(renderStats.totalTriangles)).c_str());
}

// ============================================================================
// MAIN INTEGRATION EXAMPLE
// ============================================================================

class TexturedRendererExample : public RendererPlugin {
private:
    TextureManager* m_textureManager = nullptr;
    MegaGeometryRendererTextured* m_megaGeometry = nullptr;
    
public:
    void OnLoad(ICore* core) override {
        RendererPlugin::OnLoad(core);
        
        // Initialize texture manager FIRST
        m_textureManager = new TextureManager();
        if (!m_textureManager->Initialize(m_device, core)) {
            core->GetLogger()->LogError("Renderer", "Failed to init texture manager");
            return;
        }
        
        // Initialize geometry renderer WITH texture manager
        m_megaGeometry = new MegaGeometryRendererTextured();
        if (!m_megaGeometry->Initialize(m_device, m_renderPass, core, m_textureManager)) {
            core->GetLogger()->LogError("Renderer", "Failed to init mega geometry");
            return;
        }
        
        // Run examples
        Example1_BasicTexturedCube(core, m_textureManager, m_megaGeometry);
        Example2_MultipleTextures(core, m_textureManager, m_megaGeometry);
        Example4_SpriteAtlas(core, m_textureManager, m_megaGeometry);
        Example5_PBRMaterial(core, m_textureManager, m_megaGeometry);
        Example6_BatchUpdates(core, m_textureManager, m_megaGeometry);
        Example7_TextureStats(core, m_textureManager, m_megaGeometry);
    }
    
    void OnUpdate(float dt) override {
        // Dynamic texture swapping example
        Example3_DynamicTextureSwap(m_core, m_textureManager, m_megaGeometry, dt);
        
        // Normal rendering
        RendererPlugin::OnUpdate(dt);
    }
    
    void OnUnload() override {
        delete m_megaGeometry;
        delete m_textureManager;
        RendererPlugin::OnUnload();
    }
};

// ============================================================================
// USAGE IN YOUR GAME
// ============================================================================

/*
// In your game initialization:

// 1. Load level textures
uint32_t wallTexture = textureManager->LoadTexture("levels/level1_wall.png");
uint32_t floorTexture = textureManager->LoadTexture("levels/level1_floor.png");

// 2. Create level geometry
megaGeometry->LoadMesh("levels/level1_walls.mesh", 0);
megaGeometry->LoadMesh("levels/level1_floor.mesh", 1);

// 3. Spawn textured objects
for (auto& wall : levelData.walls) {
    uint32_t instance = megaGeometry->AddInstance(
        0,  // Wall mesh
        wall.x, wall.y, wall.z,
        wallTexture
    );
}

// 4. Render each frame
void GameLoop::Render() {
    // Update camera
    megaGeometry->SetViewProjection(camera.GetViewProjectionMatrix());
    
    // Render
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    megaGeometry->PreRender(cmd);  // GPU culling
    megaGeometry->Render(cmd);     // Draw textured geometry
    vkCmdEndRenderPass(cmd);
}
*/
