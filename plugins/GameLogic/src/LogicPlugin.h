#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/IWorld.h>
#include <SecretEngine/IInputSystem.h>
#include <SecretEngine/IRenderer.h>
#include <SecretEngine/Components.h>
#include <SecretEngine/Fast/FastData.h>

namespace SecretEngine {
    using namespace Fast;

    class GameLogic : public IPlugin {
    public:
        const char* GetName() const override { return "GameLogic"; }
        uint32_t GetVersion() const override { return 1; }
        
        void OnLoad(ICore* core) override {
            m_core = core;
            core->RegisterCapability("logic", this);
        }
        
        void OnActivate() override;
        void OnDeactivate() override {}
        void OnUnload() override {}
        
        void OnUpdate(float dt) override;

        ICore* m_core = nullptr;
        int m_cubeColorIndex = 0;
        SecretEngine::Entity m_characterEntity = {0, 0};
        
        static constexpr float COLORS[5][3] = {
            {1.0f, 0.4f, 0.4f}, {0.4f, 1.0f, 0.4f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.4f}, {1.0f, 0.4f, 1.0f}
        };
    };

} // namespace SecretEngine
