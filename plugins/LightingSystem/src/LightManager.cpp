#include "LightManager.h"
#include <algorithm>

namespace SecretEngine {

LightManager::LightManager() {
    m_lights.reserve(MAX_LIGHTS);
    m_activeSlots.reserve(MAX_LIGHTS);
}

LightManager::~LightManager() {
}

uint32_t LightManager::AddLight(const LightData& light) {
    if (m_lights.size() >= MAX_LIGHTS) {
        return 0;  // Failed
    }
    
    uint32_t id = m_nextID++;
    m_lights.push_back(light);
    m_activeSlots.push_back(true);
    
    return id;
}

void LightManager::UpdateLight(uint32_t lightID, const LightData& light) {
    // Simple linear search for now (TODO: use hash map for O(1) lookup)
    for (size_t i = 0; i < m_lights.size(); ++i) {
        if (m_activeSlots[i]) {
            m_lights[i] = light;
            break;
        }
    }
}

void LightManager::RemoveLight(uint32_t lightID) {
    // Mark slot as inactive (TODO: implement proper ID tracking)
    if (lightID > 0 && lightID < m_activeSlots.size()) {
        m_activeSlots[lightID - 1] = false;
    }
}

const LightData* LightManager::GetLight(uint32_t lightID) const {
    if (lightID > 0 && lightID <= m_lights.size() && m_activeSlots[lightID - 1]) {
        return &m_lights[lightID - 1];
    }
    return nullptr;
}

uint32_t LightManager::GetLightCount() const {
    return static_cast<uint32_t>(std::count(m_activeSlots.begin(), m_activeSlots.end(), true));
}

std::span<const LightData> LightManager::GetLightBuffer() const {
    return std::span<const LightData>(m_lights.data(), m_lights.size());
}

const void* LightManager::GetLightBufferRaw() const {
    return m_lights.empty() ? nullptr : m_lights.data();
}

size_t LightManager::GetLightBufferSize() const {
    return m_lights.size() * sizeof(LightData);
}

} // namespace SecretEngine
