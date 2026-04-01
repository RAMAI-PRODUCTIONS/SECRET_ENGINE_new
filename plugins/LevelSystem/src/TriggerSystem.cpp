#include "TriggerSystem.h"
#include "V73LevelManager.h"
#include <SecretEngine/ILogger.h>
#include <cmath>

namespace SecretEngine::Levels::V73 {

TriggerSystem::TriggerSystem(V73LevelManager* levelManager)
    : m_levelManager(levelManager) {
}

void TriggerSystem::InitializeTriggers(const std::vector<Trigger>& triggers) {
    m_triggers = triggers;
    m_triggerStates.clear();
    
    for (const auto& trigger : m_triggers) {
        m_triggerStates[trigger.id] = TriggerState{};
    }
    
    if (m_debugMode) {
        // Log trigger initialization
    }
}

void TriggerSystem::Update(float deltaTime) {
    // Update cooldowns and states
    auto now = std::chrono::steady_clock::now();
    
    for (auto& [triggerId, state] : m_triggerStates) {
        if (state.hasTriggered) {
            // Find the trigger
            auto it = std::find_if(m_triggers.begin(), m_triggers.end(),
                [&triggerId](const Trigger& t) { return t.id == triggerId; });
            
            if (it != m_triggers.end() && it->repeatable) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - state.lastTriggerTime).count();
                
                if (elapsed >= it->cooldown) {
                    state.hasTriggered = false;
                }
            }
        }
    }
}

void TriggerSystem::CheckEntity(uint32_t entityId, const glm::vec3& position) {
    for (const auto& trigger : m_triggers) {
        auto& state = m_triggerStates[trigger.id];
        
        bool isInside = IsEntityInTrigger(position, trigger);
        bool wasInside = std::find(state.entitiesInside.begin(), 
                                   state.entitiesInside.end(), 
                                   entityId) != state.entitiesInside.end();
        
        if (isInside && !wasInside) {
            // Entity entered trigger
            state.entitiesInside.push_back(entityId);
            OnEntityEnter(trigger, entityId);
        } else if (!isInside && wasInside) {
            // Entity exited trigger
            state.entitiesInside.erase(
                std::remove(state.entitiesInside.begin(), 
                           state.entitiesInside.end(), 
                           entityId),
                state.entitiesInside.end());
            OnEntityExit(trigger, entityId);
        }
    }
}

void TriggerSystem::RegisterActionHandler(const std::string& actionType, 
                                         TriggerActionHandler handler) {
    m_actionHandlers[actionType] = handler;
}

void TriggerSystem::Clear() {
    m_triggers.clear();
    m_triggerStates.clear();
}

bool TriggerSystem::IsEntityInTrigger(const glm::vec3& entityPos, 
                                      const Trigger& trigger) const {
    glm::vec3 diff = entityPos - trigger.position;
    
    if (trigger.shape == "sphere") {
        float distSq = glm::dot(diff, diff);
        return distSq <= (trigger.radius * trigger.radius);
    } else if (trigger.shape == "box") {
        // Simple box check using radius as half-extents
        return std::abs(diff.x) <= trigger.radius &&
               std::abs(diff.y) <= trigger.radius &&
               std::abs(diff.z) <= trigger.radius;
    } else if (trigger.shape == "cylinder") {
        float distXZSq = diff.x * diff.x + diff.z * diff.z;
        return distXZSq <= (trigger.radius * trigger.radius) &&
               std::abs(diff.y) <= trigger.radius;
    }
    
    return false;
}

void TriggerSystem::OnEntityEnter(const Trigger& trigger, uint32_t entityId) {
    auto& state = m_triggerStates[trigger.id];
    
    if (!CanTriggerActivate(trigger, state)) {
        return;
    }
    
    state.isActive = true;
    state.hasTriggered = true;
    state.lastTriggerTime = std::chrono::steady_clock::now();
    
    ExecuteActions(trigger.enter_actions, entityId);
}

void TriggerSystem::OnEntityExit(const Trigger& trigger, uint32_t entityId) {
    auto& state = m_triggerStates[trigger.id];
    
    if (state.entitiesInside.empty()) {
        state.isActive = false;
    }
    
    ExecuteActions(trigger.exit_actions, entityId);
}

void TriggerSystem::ExecuteActions(const std::vector<Trigger::Action>& actions, 
                                   uint32_t entityId) {
    for (const auto& action : actions) {
        auto it = m_actionHandlers.find(action.type);
        if (it != m_actionHandlers.end()) {
            it->second(action, entityId);
        }
    }
}

bool TriggerSystem::CanTriggerActivate(const Trigger& trigger, 
                                       const TriggerState& state) const {
    if (state.hasTriggered && !trigger.repeatable) {
        return false;
    }
    
    if (state.hasTriggered && trigger.repeatable) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - state.lastTriggerTime).count();
        
        if (elapsed < trigger.cooldown) {
            return false;
        }
    }
    
    return true;
}

} // namespace SecretEngine::Levels::V73
