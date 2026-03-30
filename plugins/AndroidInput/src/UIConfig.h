// SecretEngine
// Module: UIConfig
// Responsibility: Runtime UI configuration loader
// Dependencies: None (standalone JSON parser)

#pragma once
#include <string>
#include <vector>
#include <map>

namespace SecretEngine {

struct UIButtonConfig {
    std::string id;
    std::string label;
    std::string action;
    int positionIndex;
    float color[3];
    float textColor[3];
    bool visible;
};

struct JoystickConfig {
    bool enabled;
    float xNDC;
    float yNDC;
    float baseSize;
    float stickSize;
    float maxOffsetMultiplier;
    float baseColor[3];
    float stickColor[3];
    float touchRadiusPixels;
    bool visible;
};

struct ScreenZoneConfig {
    bool uiButtonsEnabled;
    float uiButtonsHeightPercent;
    bool joystickEnabled;
    bool lookEnabled;
};

struct TouchInputConfig {
    float joystickSensitivity;
    float lookSensitivity;
    bool fireOnTap;
};

struct LayoutConfig {
    float buttonInset;
    float buttonTextScale;
    float buttonTextSpacing;
    float buttonTextYOffset;
};

class UIConfig {
public:
    UIConfig();
    
    // Load configuration from JSON file
    bool LoadFromFile(const char* filepath);
    
    // Getters
    const std::vector<UIButtonConfig>& GetButtons() const { return m_buttons; }
    const JoystickConfig& GetJoystick() const { return m_joystick; }
    const ScreenZoneConfig& GetScreenZones() const { return m_screenZones; }
    const TouchInputConfig& GetTouchInput() const { return m_touchInput; }
    const LayoutConfig& GetLayout() const { return m_layout; }
    
    // Hot reload support
    bool NeedsReload() const;
    void MarkReloaded();
    
private:
    std::vector<UIButtonConfig> m_buttons;
    JoystickConfig m_joystick;
    ScreenZoneConfig m_screenZones;
    TouchInputConfig m_touchInput;
    LayoutConfig m_layout;
    
    std::string m_filepath;
    long long m_lastModifiedTime;
    
    // Simple JSON parsing helpers
    bool ParseJSON(const std::string& content);
    std::string ReadFile(const char* filepath);
    long long GetFileModifiedTime(const char* filepath) const;
};

} // namespace SecretEngine
