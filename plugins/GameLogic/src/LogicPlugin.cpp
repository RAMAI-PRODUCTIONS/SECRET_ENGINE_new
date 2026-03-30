#include "LogicPlugin.h"
#include <SecretEngine/ILogger.h>

namespace SecretEngine {

void GameLogic::OnActivate() {
    m_core->GetLogger()->LogInfo("GameLogic", "Logic Plugin Activated");
}

void GameLogic::OnUpdate(float dt) {
    auto input = m_core->GetInput();
    auto world = m_core->GetWorld();
    if (!input || !world) return;

    // Detect character entity if not found yet
    if (m_characterEntity.id == 0) {
        const auto& entities = world->GetAllEntities();
        for (auto e : entities) {
            // In the renderer code, m_characterEntity was set if isPlayer was true in scene.json
            // We can't easily check isPlayer here unless we re-parse or add a component.
            // For now, let's assume the first entity with a MeshComponent is our target for the demo
            if (world->GetComponent(e, MeshComponent::TypeID)) {
                m_characterEntity = e;
                break;
            }
        }
    }

    // Interaction Logic: Change color on touch/click
    static bool wasTouched = false;
    if (input->IsMouseButtonPressed(0)) {
        if (!wasTouched) {
            m_cubeColorIndex = (m_cubeColorIndex + 1) % 5;
            
            if (m_characterEntity.id != 0) {
                auto mesh = static_cast<MeshComponent*>(world->GetComponent(m_characterEntity, MeshComponent::TypeID));
                if (mesh) {
                    mesh->color[0] = COLORS[m_cubeColorIndex][0];
                    mesh->color[1] = COLORS[m_cubeColorIndex][1];
                    mesh->color[2] = COLORS[m_cubeColorIndex][2];
                    mesh->color[3] = 1.0f;
                    
                    m_core->GetLogger()->LogInfo("GameLogic", "Input Detected: Cycling Mesh Color");
                }
            }
            wasTouched = true;
        }
    } else {
        wasTouched = false;
    }
}

extern "C" IPlugin* CreateLogicPlugin() {
    return new GameLogic();
}

} // namespace SecretEngine
