// SecretEngine
// Module: GameplayTagPlugin
// Responsibility: Plugin wrapper for Gameplay Tag System
// Dependencies: Core, GameplayAbilitySystem

#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/ILogger.h>
#include "GameplayAbilitySystem.h"

namespace SecretEngine::GAS {

class GameplayTagPlugin : public IPlugin {
public:
    void OnLoad(ICore* core) override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void* GetInterface(uint32_t interfaceId) override { return nullptr; }
    const char* GetName() const override { return "GameplayTagSystem"; }
    uint32_t GetVersion() const override { return 1; }
    
    // Access to data table
    GameplayDataTable* GetDataTable() { return m_dataTable; }
    
private:
    ICore* m_core = nullptr;
    IWorld* m_world = nullptr;
    ILogger* m_logger = nullptr;
    GameplayDataTable* m_dataTable = nullptr;
};

} // namespace SecretEngine::GAS
