#include "MaterialPlugin.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <cstring>

MaterialPlugin::~MaterialPlugin() {
}

void MaterialPlugin::OnLoad(SecretEngine::ICore* core) {
    m_core = core;
    m_materials.reserve(MAX_MATERIALS);
    
    // Register capability
    m_core->RegisterCapability("materials", this);
    
    if (m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("MaterialSystem", "✓ GPU-Driven Material System initialized (Bindless, 4096 materials)");
    }
}

void MaterialPlugin::OnActivate() {
    // Create default material
    SecretEngine::MaterialProperties defaultMat = {};
    defaultMat.baseColor[0] = 1.0f;
    defaultMat.baseColor[1] = 1.0f;
    defaultMat.baseColor[2] = 1.0f;
    defaultMat.baseColor[3] = 1.0f;
    defaultMat.metallic = 0.0f;
    defaultMat.roughness = 0.5f;
    defaultMat.albedoTexture = 0xFFFFFFFF;  // No texture
    
    CreateMaterial("default", defaultMat);
}

void MaterialPlugin::OnDeactivate() {
}

void MaterialPlugin::OnUnload() {
}

void MaterialPlugin::OnUpdate(float deltaTime) {
}

SecretEngine::MaterialHandle MaterialPlugin::CreateMaterial(const char* name, const SecretEngine::MaterialProperties& props) {
    uint32_t index;
    
    // Reuse free slot or allocate new
    if (!m_freeSlots.empty()) {
        index = m_freeSlots.back();
        m_freeSlots.pop_back();
        m_materials[index] = props;
    } else {
        if (m_materials.size() >= MAX_MATERIALS) {
            return {0, 0};  // Failed
        }
        index = static_cast<uint32_t>(m_materials.size());
        m_materials.push_back(props);
    }
    
    m_nameToIndex[name] = index;
    
    return {index, m_nextGeneration++};
}

void MaterialPlugin::UpdateMaterial(SecretEngine::MaterialHandle handle, const SecretEngine::MaterialProperties& props) {
    if (handle.id < m_materials.size()) {
        m_materials[handle.id] = props;
    }
}

void MaterialPlugin::DestroyMaterial(SecretEngine::MaterialHandle handle) {
    if (handle.id < m_materials.size()) {
        m_freeSlots.push_back(handle.id);
    }
}

const SecretEngine::MaterialProperties* MaterialPlugin::GetMaterial(SecretEngine::MaterialHandle handle) const {
    return (handle.id < m_materials.size()) ? &m_materials[handle.id] : nullptr;
}

SecretEngine::MaterialHandle MaterialPlugin::GetMaterialByName(const char* name) const {
    auto it = m_nameToIndex.find(name);
    if (it != m_nameToIndex.end()) {
        return {it->second, 0};
    }
    return {0, 0};
}

std::span<const SecretEngine::MaterialProperties> MaterialPlugin::GetMaterialBuffer() const {
    return std::span<const SecretEngine::MaterialProperties>(m_materials.data(), m_materials.size());
}

const void* MaterialPlugin::GetMaterialBufferRaw() const {
    return m_materials.empty() ? nullptr : m_materials.data();
}

size_t MaterialPlugin::GetMaterialBufferSize() const {
    return m_materials.size() * sizeof(SecretEngine::MaterialProperties);
}

uint32_t MaterialPlugin::GetMaterialCount() const {
    return static_cast<uint32_t>(m_materials.size());
}

extern "C" {
    SecretEngine::IPlugin* CreatePlugin() {
        return new MaterialPlugin();
    }
    
    void DestroyPlugin(SecretEngine::IPlugin* plugin) {
        delete plugin;
    }
}
