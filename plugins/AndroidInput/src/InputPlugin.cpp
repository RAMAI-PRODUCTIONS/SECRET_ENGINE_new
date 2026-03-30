#include "InputPlugin.h"
#include "../../FPSGameLogic/src/FPSGamePlugin.h"
#include "../../FPSGameLogic/src/FPSFastData.h"
#include "../../FPSUIPlugin/src/FPSUIPlugin.h"

namespace SecretEngine {

void AndroidInput::SendFireActionToFPSGame() {
    auto* fpsGamePlugin = m_core->GetCapability("fps_game");
    if (!fpsGamePlugin) {
        if (m_core->GetLogger()) {
            m_core->GetLogger()->LogInfo("AndroidInput", "🔫 Fire action - FPS game not loaded");
        }
        return;
    }
    
    // Cast to FPSGamePlugin and push ACTION_FIRE packet
    auto* fpsGame = static_cast<SecretEngine::FPS::FPSGamePlugin*>(fpsGamePlugin);
    auto& streams = fpsGame->GetFastStreams();
    auto playerEntity = fpsGame->GetLocalPlayerEntity();
    
    if (m_core->GetLogger()) {
        char msg[128];
        snprintf(msg, sizeof(msg), "🔫 FIRE ACTION - Sending to entity %u", playerEntity.id);
        m_core->GetLogger()->LogInfo("AndroidInput", msg);
    }
    
    // Create and push fire action packet
    SecretEngine::FPS::Fast::PlayerActionPacket firePacket;
    firePacket.entityId = playerEntity.id;
    firePacket.action = 0; // ACTION_FIRE
    firePacket.sequence = 0;
    firePacket.padding[0] = firePacket.padding[1] = firePacket.padding[2] = 0;
    
    streams.actionInput.Push(firePacket);
}

void AndroidInput::SendUIButtonPress(const char* buttonName) {
    auto* fpsUIPlugin = m_core->GetCapability("fps_ui");
    if (!fpsUIPlugin) {
        if (m_core->GetLogger()) {
            m_core->GetLogger()->LogWarning("AndroidInput", "UI plugin not found");
        }
        return;
    }
    
    // Cast to FPSUIPlugin and trigger button press
    auto* fpsUI = static_cast<SecretEngine::FPSUI::FPSUIPlugin*>(fpsUIPlugin);
    fpsUI->OnButtonPress(buttonName);
}

} // namespace SecretEngine

// Factory function implementation
extern "C" {
    SecretEngine::IPlugin* CreateInputPlugin() {
        return new SecretEngine::AndroidInput();
    }
}

