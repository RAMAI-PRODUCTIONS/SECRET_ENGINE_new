#pragma once
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/Components.h>
#include "MegaGeometryRenderer.h"
#include "TextureManager.h"
#include <unordered_map>

namespace SecretEngine {

class MeshRenderingSystem {
public:
    MeshRenderingSystem(ICore* core, MegaGeometry::MegaGeometryRenderer* megaGeometry, Textures::TextureManager* textureManager);
    ~MeshRenderingSystem();
    
    void Update(float deltaTime);
    
private:
    ICore* m_core;
    IWorld* m_world;
    ILogger* m_logger;
    MegaGeometry::MegaGeometryRenderer* m_megaGeometry;
    Textures::TextureManager* m_textureManager;
    
    // Track which entities have been added to renderer
    std::unordered_map<uint32_t, uint32_t> m_entityToInstanceMap; // Entity ID -> Instance ID
    
    void ProcessNewEntities();
    void UpdateExistingEntities();
};

} // namespace SecretEngine
