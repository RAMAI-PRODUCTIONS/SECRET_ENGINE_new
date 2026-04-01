#pragma once

#include "V73LevelSystem.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace SecretEngine::Levels::V73 {

// Forward declarations
class V73LevelManager;

// Trigger state tracking
struct TriggerState {
    bool isActive = false;
    bool hasTriggered = false;
    std::chrono::steady_clock::time_point lastTriggerTime;
    std::vector<uint32_t> entitiesInside;
};

// Trigger action handler
using TriggerActionHandler = std::function<void(const Trigger::Action&, uint32_t entityId)>;

class TriggerSystem {
public:
    TriggerSystem(V73LevelManager* levelManager);
    ~TriggerSystem() = default;

    // Initialize triggers from level data
    void InitializeTriggers(const std::vector<Trigger>& triggers);
    
    // Update trigger states
    void Update(float deltaTime);
    
    // Check entity against all triggers
    void CheckEntity(uint32_t entityId, const glm::vec3& position);
    
    // Register action handlers
    void RegisterActionHandler(const std::string& actionType, TriggerActionHandler handler);
    
    // Clear all triggers
    void Clear();
    
    // Debug
    void SetDebugMode(bool enabled) { m_debugMode = enabled; }
    
private:
    V73LevelManager* m_levelManager;
    
    std::vector<Trigger> m_triggers;
    std::unordered_map<std::string, TriggerState> m_triggerStates;
    std::unordered_map<std::string, TriggerActionHandler> m_actionHandlers;
    
    bool m_debugMode = false;
    
    // Helper methods
    bool IsEntityInTrigger(const glm::vec3& entityPos, const Trigger& trigger) const;
    void OnEntityEnter(const Trigger& trigger, uint32_t entityId);
    void OnEntityExit(const Trigger& trigger, uint32_t entityId);
    void ExecuteActions(const std::vector<Trigger::Action>& actions, uint32_t entityId);
    bool CanTriggerActivate(const Trigger& trigger, const TriggerState& state) const;
};

} // namespace SecretEngine::Levels::V73
