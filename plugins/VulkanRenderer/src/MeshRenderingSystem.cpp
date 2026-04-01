#include "MeshRenderingSystem.h"
#include <cstring>

namespace SecretEngine {

MeshRenderingSystem::MeshRenderingSystem(ICore* core, MegaGeometry::MegaGeometryRenderer* megaGeometry, Textures::TextureManager* textureManager)
    : m_core(core)
    , m_world(core->GetWorld())
    , m_logger(core->GetLogger())
    , m_megaGeometry(megaGeometry)
    , m_textureManager(textureManager)
{
    m_logger->LogInfo("MeshRenderingSystem", "Initialized");
}

MeshRenderingSystem::~MeshRenderingSystem() {
}

void MeshRenderingSystem::Update(float deltaTime) {
    ProcessNewEntities();
    UpdateExistingEntities();
}

void MeshRenderingSystem::ProcessNewEntities() {
    if (!m_world || !m_megaGeometry) return;
    
    // Query all entities with both MeshComponent and TransformComponent
    auto entities = m_world->GetAllEntities();
    
    for (Entity entity : entities) {
        uint32_t entityId = entity.id;
        
        // Skip if already processed
        if (m_entityToInstanceMap.find(entityId) != m_entityToInstanceMap.end()) {
            continue;
        }
        
        // Check if entity has required components
        auto* meshComp = static_cast<MeshComponent*>(m_world->GetComponent(entity, MeshComponent::TypeID));
        auto* transformComp = static_cast<TransformComponent*>(m_world->GetComponent(entity, TransformComponent::TypeID));
        
        if (!meshComp || !transformComp) {
            continue;
        }
        
        // Load mesh if needed (assume Character.meshbin for now, slot 0)
        // In production, you'd map mesh paths to slots
        uint32_t meshSlot = 0;
        
        // Load default texture
        uint32_t textureID = UINT32_MAX;
        if (m_textureManager) {
            textureID = m_textureManager->LoadTexture("textures/diffuse.jpeg");
        }
        
        // Add instance to renderer
        uint32_t instanceID = m_megaGeometry->AddInstance(
            meshSlot,
            transformComp->position[0],
            transformComp->position[1],
            transformComp->position[2],
            textureID
        );
        
        if (instanceID < 65536) { // Valid instance
            m_entityToInstanceMap[entityId] = instanceID;
            
            // Update full transform (position + rotation)
            m_megaGeometry->UpdateInstanceTransform(instanceID, 
                transformComp->position[0],
                transformComp->position[1],
                transformComp->position[2],
                transformComp->rotation[1], // rotY
                transformComp->rotation[0], // rotX
                transformComp->rotation[2]  // rotZ
            );
            
            // Set color from mesh component
            m_megaGeometry->UpdateInstanceColor(instanceID,
                meshComp->color[0],
                meshComp->color[1],
                meshComp->color[2],
                meshComp->color[3]
            );
            
            char msg[256];
            snprintf(msg, sizeof(msg), "Added entity %u to renderer as instance %u at (%.1f, %.1f, %.1f)",
                entityId, instanceID, transformComp->position[0], transformComp->position[1], transformComp->position[2]);
            m_logger->LogInfo("MeshRenderingSystem", msg);
        }
    }
}

void MeshRenderingSystem::UpdateExistingEntities() {
    if (!m_world || !m_megaGeometry) return;
    
    // Update transforms for existing instances
    for (const auto& pair : m_entityToInstanceMap) {
        uint32_t entityId = pair.first;
        uint32_t instanceId = pair.second;
        
        auto* transformComp = static_cast<TransformComponent*>(
            m_world->GetComponent(Entity{entityId}, TransformComponent::TypeID)
        );
        
        if (transformComp) {
            m_megaGeometry->UpdateInstanceTransform(instanceId,
                transformComp->position[0],
                transformComp->position[1],
                transformComp->position[2],
                transformComp->rotation[1], // rotY
                transformComp->rotation[0], // rotX
                transformComp->rotation[2]  // rotZ
            );
        }
    }
}

} // namespace SecretEngine
