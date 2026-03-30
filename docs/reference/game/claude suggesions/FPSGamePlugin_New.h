// SecretEngine
// Module: FPSGamePlugin
// Responsibility: FPS game logic orchestration and system execution
// Dependencies: Core, FPSComponents, FPSSystems, FPSFastData

#pragma once

#include "SecretEngine/Core/IPlugin.h"
#include "SecretEngine/Core/IWorld.h"
#include "FPSComponents.h"
#include "FPSSystems.h"
#include "FPSFastData.h"

namespace SecretEngine::FPS {

// ============================================================================
// FPS GAME LOGIC PLUGIN (Header-Only for Performance)
// ============================================================================

class FPSGamePlugin : public IPlugin {
public:
    FPSGamePlugin() = default;
    ~FPSGamePlugin() override = default;
    
    // IPlugin interface
    const char* GetName() const override { return "FPSGameLogic"; }
    uint32_t GetVersion() const override { return 1; }
    
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float deltaTime) override;
    void* GetInterface(uint32_t id) override { return nullptr; }
    
    // Fast Data API
    Fast::FPSFastStreams& GetFastStreams() { return m_fastStreams; }
    
    // Match API
    MatchState& GetMatchState() { return m_matchState; }
    const MatchState& GetMatchState() const { return m_matchState; }
    
    // Player API
    uint32_t GetLocalPlayerEntity() const { return m_localPlayerEntity; }
    
    // Match control
    void StartMatch(uint32_t redBotCount, uint32_t blueBotCount);
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
    ILinearAllocator* m_frameArena = nullptr;
    
    MatchState m_matchState;
    SpawnSystemState m_spawnState;
    float m_gameTime = 0.0f;
    
    Fast::FPSFastStreams m_fastStreams;
    uint32_t m_localPlayerEntity = 0;
    
    void RegisterComponents();
    void InitializeMatchState();
    void CreatePlayerEntity();
    void CreateBotEntities(TeamComponent::Team team, uint32_t count);
    void CreateSpawnPoints();
};

} // namespace SecretEngine::FPS
