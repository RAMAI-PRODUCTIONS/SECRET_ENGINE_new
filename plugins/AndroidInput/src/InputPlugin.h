#pragma once
#include <SecretEngine/IPlugin.h>
#include <SecretEngine/IInputSystem.h>
#include <SecretEngine/ICore.h>
#include <cmath>
#include <algorithm>
#include <map>
#include "UIConfig.h"

namespace SecretEngine {
    class AndroidInput : public IPlugin, public IInputSystem {
    public:
        const char* GetName() const override { return "AndroidInput"; }
        uint32_t GetVersion() const override { return 1; }
        
        void OnLoad(ICore* core) override {
            m_core = core;
            core->RegisterCapability("input", this);
            
            // Load UI configuration
            if (!m_uiConfig.LoadFromFile("ui_config.json")) {
                // Failed to load, using defaults (no logging needed here)
            }
        }

        void OnActivate() override {}
        void OnDeactivate() override {}
        void OnUnload() override {}
        
        void OnUpdate(float dt) override {
            // Hot reload UI config
            m_configCheckTimer += dt;
            if (m_configCheckTimer > 2.0f) {
                if (m_uiConfig.NeedsReload()) {
                    m_uiConfig.LoadFromFile("ui_config.json");
                }
                m_configCheckTimer = 0.0f;
            }
            
            // Process shooting action for FPS game
            if (m_rightSideTapped) {
                SendFireActionToFPSGame();
                m_rightSideTapped = false;
            }
            
            // Process UI button presses
            if (m_uiButtonPressed[0] != '\0') {
                SendUIButtonPress(m_uiButtonPressed);
                m_uiButtonPressed[0] = '\0';
            }
        }

        virtual void* GetInterface(uint32_t id) override {
            if (id == 2) return static_cast<IInputSystem*>(this);
            return nullptr;
        }

        // IInputSystem
        void Update() override {
            // No-op for now, we want Direct Fast Consumption by other plugins
            // But we can update fallback state if needed without popping
        }

        Fast::UltraRingBuffer<512>& GetFastStream() override { return m_fastStream; }

        bool IsKeyPressed(int key) override { return false; }
        bool IsMouseButtonPressed(int button) override { return m_isLookDown || m_isJoyDown; }
        void GetMousePosition(float& x, float& y) override { x = m_lastLookX; y = m_lastLookY; }
        
        // Get joystick position for rendering (normalized -1 to 1)
        void GetJoystickPosition(float& x, float& y) const {
            x = m_joyNormalizedX;
            y = m_joyNormalizedY;
        }
        
        bool IsJoystickActive() const { return m_isJoyDown; }
        
        // Get UI configuration
        const UIConfig& GetUIConfig() const { return m_uiConfig; }

        // Advanced Android native entry (Producer)
        // action: AMOTION_EVENT_ACTION_...
        // x, y: RAW screen coordinates
        // pointerId: Unique ID for this finger
        void HandleTouch(int action, float x, float y, int pointerId, float screenWidth, float screenHeight) {
            int actionCode = action & 0xff; // AMOTION_EVENT_ACTION_MASK

            if (actionCode == 0 /* DOWN */ || actionCode == 5 /* POINTER_DOWN */) {
                // Check for UI button zones (configurable)
                const auto& zones = m_uiConfig.GetScreenZones();
                if (zones.uiButtonsEnabled && y < screenHeight * (zones.uiButtonsHeightPercent / 100.0f)) {
                    // UI button zone - find which button was pressed
                    const auto& buttons = m_uiConfig.GetButtons();
                    int visibleCount = 0;
                    for (const auto& btn : buttons) {
                        if (btn.visible) visibleCount++;
                    }
                    
                    if (visibleCount > 0) {
                        float buttonWidth = screenWidth / static_cast<float>(visibleCount);
                        int buttonIndex = static_cast<int>(x / buttonWidth);
                        
                        // Find the button at this index
                        int currentIndex = 0;
                        for (const auto& btn : buttons) {
                            if (btn.visible) {
                                if (currentIndex == buttonIndex) {
                                    strncpy(m_uiButtonPressed, btn.action.c_str(), 64);
                                    break;
                                }
                                currentIndex++;
                            }
                        }
                    }
                    return; // Don't process as game input
                }
                
                // Check joystick and look zones
                if (zones.joystickEnabled && x < screenWidth / 2.0f) {
                    // Left side - Joystick
                    if (m_joyPointerId == -1) {
                        m_joyPointerId = pointerId;
                        m_joyStartX = x;
                        m_joyStartY = y;
                        m_isJoyDown = true;
                    }
                } else if (zones.lookEnabled && x >= screenWidth / 2.0f) {
                    // Right side - Look AND Shoot
                    if (m_lookPointerId == -1) {
                        m_lookPointerId = pointerId;
                        m_lastLookX = x;
                        m_lastLookY = y;
                        m_isLookDown = true;
                        
                        // TRIGGER SHOOT on tap (configurable)
                        const auto& touchInput = m_uiConfig.GetTouchInput();
                        if (touchInput.fireOnTap) {
                            m_rightSideTapped = true;
                            
                            // Send shoot button press (metadata = 2 for fire button)
                            Fast::UltraPacket p;
                            p.Set(Fast::PacketType::InputAction, 2, 1, 0); // button=2 (fire), pressed=1
                            m_fastStream.Push(p);
                        }
                    }
                }
            }
            else if (actionCode == 2 /* MOVE */) {
                if (pointerId == m_joyPointerId) {
                    float dx = x - m_joyStartX;
                    float dy = y - m_joyStartY;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    
                    // Use configurable radius
                    const auto& joyConfig = m_uiConfig.GetJoystick();
                    float radius = joyConfig.touchRadiusPixels;

                    if (dist > radius) {
                        dx *= radius / dist;
                        dy *= radius / dist;
                    }
                    
                    // Store normalized joystick position for rendering
                    m_joyNormalizedX = dx / radius;
                    m_joyNormalizedY = dy / radius;

                    // Apply sensitivity from config
                    const auto& touchInput = m_uiConfig.GetTouchInput();
                    float sensitivity = touchInput.joystickSensitivity;
                    
                    // Send Movement Axis (metadata = 0)
                    Fast::UltraPacket p;
                    p.Set(Fast::PacketType::InputAxis, 0, 
                          static_cast<int16_t>((dx / radius) * 32767.0f * sensitivity),
                          static_cast<int16_t>((dy / radius) * 32767.0f * sensitivity));
                    m_fastStream.Push(p);
                } 
                else if (pointerId == m_lookPointerId) {
                    float dx = x - m_lastLookX;
                    float dy = y - m_lastLookY;
                    
                    // Normalize delta relative to screen size for consistency
                    float normDX = dx / screenWidth;
                    float normDY = dy / screenHeight;

                    // Apply configurable sensitivity
                    const auto& touchInput = m_uiConfig.GetTouchInput();
                    float sensitivity = touchInput.lookSensitivity;
                    
                    // Send Look Axis (metadata = 1)
                    Fast::UltraPacket p;
                    p.Set(Fast::PacketType::InputAxis, 1, 
                          static_cast<int16_t>(normDX * 32767.0f * sensitivity),
                          static_cast<int16_t>(normDY * 32767.0f * sensitivity));
                    m_fastStream.Push(p);

                    m_lastLookX = x;
                    m_lastLookY = y;
                }
            }
            else if (actionCode == 1 /* UP */ || actionCode == 6 /* POINTER_UP */ || actionCode == 3 /* CANCEL */) {
                if (pointerId == m_joyPointerId) {
                    m_joyPointerId = -1;
                    m_isJoyDown = false;
                    m_joyNormalizedX = 0.0f;
                    m_joyNormalizedY = 0.0f;
                    // Reset movement axis
                    Fast::UltraPacket p;
                    p.Set(Fast::PacketType::InputAxis, 0, 0, 0);
                    m_fastStream.Push(p);
                }
                else if (pointerId == m_lookPointerId) {
                    m_lookPointerId = -1;
                    m_isLookDown = false;
                }
            }
        }

    private:
        ICore* m_core = nullptr;
        Fast::UltraRingBuffer<512> m_fastStream;
        
        // UI Configuration
        UIConfig m_uiConfig;
        float m_configCheckTimer = 0.0f;
        
        // Joystick state
        int m_joyPointerId = -1;
        float m_joyStartX = 0, m_joyStartY = 0;
        bool m_isJoyDown = false;
        float m_joyNormalizedX = 0.0f; // -1 to 1
        float m_joyNormalizedY = 0.0f; // -1 to 1

        // Look state
        int m_lookPointerId = -1;
        float m_lastLookX = 0, m_lastLookY = 0;
        bool m_isLookDown = false;
        
        // Shoot state
        bool m_rightSideTapped = false;
        
        // UI button state
        char m_uiButtonPressed[64] = "";
        
        // Helper to send fire action to FPS game
        void SendFireActionToFPSGame();
        
        // Helper to send UI button press
        void SendUIButtonPress(const char* buttonName);
    };

    // Factory function declaration
    extern "C" IPlugin* CreateInputPlugin();
}
