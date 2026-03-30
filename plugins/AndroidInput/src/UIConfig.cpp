// SecretEngine
// Module: UIConfig
// Responsibility: Runtime UI configuration implementation
// Dependencies: Standard library only

#include "UIConfig.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

namespace SecretEngine {

UIConfig::UIConfig() : m_lastModifiedTime(0) {
    // Set defaults
    m_joystick.enabled = true;
    m_joystick.xNDC = -0.7f;
    m_joystick.yNDC = 0.5f;
    m_joystick.baseSize = 0.3f;
    m_joystick.stickSize = 0.1f;
    m_joystick.maxOffsetMultiplier = 0.5f;
    m_joystick.baseColor[0] = 0.2f; m_joystick.baseColor[1] = 0.2f; m_joystick.baseColor[2] = 0.2f;
    m_joystick.stickColor[0] = 1.0f; m_joystick.stickColor[1] = 1.0f; m_joystick.stickColor[2] = 1.0f;
    m_joystick.touchRadiusPixels = 100.0f;
    m_joystick.visible = true;
    
    m_screenZones.uiButtonsEnabled = true;
    m_screenZones.uiButtonsHeightPercent = 15.0f;
    m_screenZones.joystickEnabled = true;
    m_screenZones.lookEnabled = true;
    
    m_touchInput.joystickSensitivity = 1.0f;
    m_touchInput.lookSensitivity = 10.0f;
    m_touchInput.fireOnTap = true;
    
    m_layout.buttonInset = 0.01f;
    m_layout.buttonTextScale = 0.035f;
    m_layout.buttonTextSpacing = 0.045f;
    m_layout.buttonTextYOffset = 0.3f;
}

std::string UIConfig::ReadFile(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

long long UIConfig::GetFileModifiedTime(const char* filepath) const {
#ifdef _WIN32
    struct _stat64 fileInfo;
    if (_stat64(filepath, &fileInfo) != 0) return 0;
    return fileInfo.st_mtime;
#else
    struct stat fileInfo;
    if (stat(filepath, &fileInfo) != 0) return 0;
    return fileInfo.st_mtime;
#endif
}

bool UIConfig::LoadFromFile(const char* filepath) {
    m_filepath = filepath;
    std::string content = ReadFile(filepath);
    if (content.empty()) return false;
    
    m_lastModifiedTime = GetFileModifiedTime(filepath);
    return ParseJSON(content);
}

bool UIConfig::NeedsReload() const {
    if (m_filepath.empty()) return false;
    long long currentTime = GetFileModifiedTime(m_filepath.c_str());
    return currentTime > m_lastModifiedTime;
}

void UIConfig::MarkReloaded() {
    m_lastModifiedTime = GetFileModifiedTime(m_filepath.c_str());
}

// Simple JSON parser (minimal implementation)
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

static std::string extractString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    
    pos = json.find("\"", pos);
    if (pos == std::string::npos) return "";
    
    size_t end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";
    
    return json.substr(pos + 1, end - pos - 1);
}

static float extractFloat(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return 0.0f;
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return 0.0f;
    
    pos = json.find_first_of("-0123456789", pos);
    if (pos == std::string::npos) return 0.0f;
    
    return std::stof(json.substr(pos));
}

static bool extractBool(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return false;
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return false;
    
    size_t truePos = json.find("true", pos);
    size_t falsePos = json.find("false", pos);
    
    if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos)) {
        return true;
    }
    return false;
}

static void extractArray3(const std::string& json, const std::string& key, float* out) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return;
    
    pos = json.find("[", pos);
    if (pos == std::string::npos) return;
    
    size_t end = json.find("]", pos);
    if (end == std::string::npos) return;
    
    std::string arrayContent = json.substr(pos + 1, end - pos - 1);
    std::stringstream ss(arrayContent);
    std::string item;
    int idx = 0;
    
    while (std::getline(ss, item, ',') && idx < 3) {
        out[idx++] = std::stof(trim(item));
    }
}

bool UIConfig::ParseJSON(const std::string& content) {
    // Parse screen zones
    m_screenZones.uiButtonsEnabled = extractBool(content, "enabled");
    m_screenZones.uiButtonsHeightPercent = extractFloat(content, "height_percent");
    
    // Parse joystick
    m_joystick.enabled = extractBool(content, "enabled");
    m_joystick.xNDC = extractFloat(content, "x_ndc");
    m_joystick.yNDC = extractFloat(content, "y_ndc");
    m_joystick.baseSize = extractFloat(content, "base_size");
    m_joystick.stickSize = extractFloat(content, "stick_size");
    m_joystick.maxOffsetMultiplier = extractFloat(content, "max_offset_multiplier");
    m_joystick.touchRadiusPixels = extractFloat(content, "touch_radius_pixels");
    m_joystick.visible = extractBool(content, "visible");
    
    extractArray3(content, "base", m_joystick.baseColor);
    extractArray3(content, "stick", m_joystick.stickColor);
    
    // Parse touch input
    m_touchInput.joystickSensitivity = extractFloat(content, "joystick_sensitivity");
    m_touchInput.lookSensitivity = extractFloat(content, "look_sensitivity");
    m_touchInput.fireOnTap = extractBool(content, "fire_on_tap");
    
    // Parse layout
    m_layout.buttonInset = extractFloat(content, "button_inset");
    m_layout.buttonTextScale = extractFloat(content, "button_text_scale");
    m_layout.buttonTextSpacing = extractFloat(content, "button_text_spacing");
    m_layout.buttonTextYOffset = extractFloat(content, "button_text_y_offset");
    
    // Parse buttons array
    m_buttons.clear();
    size_t btnArrayPos = content.find("\"ui_buttons\"");
    if (btnArrayPos != std::string::npos) {
        size_t arrayStart = content.find("[", btnArrayPos);
        size_t arrayEnd = content.find("]", arrayStart);
        
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string buttonsArray = content.substr(arrayStart, arrayEnd - arrayStart);
            
            // Find each button object
            size_t objPos = 0;
            while ((objPos = buttonsArray.find("{", objPos)) != std::string::npos) {
                size_t objEnd = buttonsArray.find("}", objPos);
                if (objEnd == std::string::npos) break;
                
                std::string btnObj = buttonsArray.substr(objPos, objEnd - objPos + 1);
                
                UIButtonConfig btn;
                btn.id = extractString(btnObj, "id");
                btn.label = extractString(btnObj, "label");
                btn.action = extractString(btnObj, "action");
                btn.positionIndex = static_cast<int>(extractFloat(btnObj, "position_index"));
                btn.visible = extractBool(btnObj, "visible");
                
                extractArray3(btnObj, "color", btn.color);
                extractArray3(btnObj, "text_color", btn.textColor);
                
                m_buttons.push_back(btn);
                objPos = objEnd + 1;
            }
        }
    }
    
    return true;
}

} // namespace SecretEngine
