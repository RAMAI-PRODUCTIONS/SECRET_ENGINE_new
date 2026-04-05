// ============================================================================
// MEGA LIGHT RENDERER - Implementation
// Zero-copy persistent-mapped light SSBO + FDA lock-free update stream
// ============================================================================
#include "MegaLightRenderer.h"
#include <cstring>
#include <vulkan/vulkan.h>

namespace SecretEngine {

static uint32_t FindMemType(VkPhysicalDevice phys, uint32_t bits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    return UINT32_MAX;
}

bool MegaLightRenderer::InitWithHandles(VkDevice dev, VkPhysicalDevice phys) {
    m_device = dev;
    VkDeviceSize bufSize = sizeof(GPULightData) * MAX_LIGHTS;

    for (int i = 0; i < 2; ++i) {
        VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size  = bufSize;
        bi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(dev, &bi, nullptr, &m_lightSSBO[i]) != VK_SUCCESS) return false;

        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(dev, m_lightSSBO[i], &req);

        VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize  = req.size;
        ai.memoryTypeIndex = FindMemType(phys, req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (ai.memoryTypeIndex == UINT32_MAX) return false;

        if (vkAllocateMemory(dev, &ai, nullptr, &m_lightSSBOMemory[i]) != VK_SUCCESS) return false;
        vkBindBufferMemory(dev, m_lightSSBO[i], m_lightSSBOMemory[i], 0);
        vkMapMemory(dev, m_lightSSBOMemory[i], 0, bufSize, 0, (void**)&m_mapped[i]);
        memset(m_mapped[i], 0, bufSize);
    }
    return true;
}

void MegaLightRenderer::Shutdown() {
    for (int i = 0; i < 2; ++i) {
        if (m_lightSSBOMemory[i]) vkFreeMemory(m_device, m_lightSSBOMemory[i], nullptr);
        if (m_lightSSBO[i])       vkDestroyBuffer(m_device, m_lightSSBO[i], nullptr);
        m_mapped[i] = nullptr;
        m_lightSSBO[i] = VK_NULL_HANDLE;
        m_lightSSBOMemory[i] = VK_NULL_HANDLE;
    }
}

uint32_t MegaLightRenderer::AddLight(const LightData& light) {
    uint32_t idx = m_lightCount.fetch_add(1, std::memory_order_relaxed);
    if (idx >= MAX_LIGHTS) { m_lightCount.fetch_sub(1, std::memory_order_relaxed); return UINT32_MAX; }
    for (int b = 0; b < 2; ++b) {
        if (!m_mapped[b]) continue;
        GPULightData& g = m_mapped[b][idx];
        g.type        = (int)light.type;
        g.posX        = light.position[0];  g.posY = light.position[1];  g.posZ = light.position[2];
        g.dirX        = light.direction[0]; g.dirY = light.direction[1]; g.dirZ = light.direction[2];
        g.colR        = light.color[0];     g.colG = light.color[1];     g.colB = light.color[2];
        g.intensity   = light.intensity;
        g.range       = light.range;
        g.spotAngle   = light.spotAngle;
        g.constAtten  = light.constantAttenuation;
        g.linearAtten = light.linearAttenuation;
        g.quadAtten   = light.quadraticAttenuation;
        g.padding     = 0;
    }
    return idx;
}

void MegaLightRenderer::PushLightUpdate(uint32_t idx, float x, float y, float z,
                                         float r, float g, float b, float intensity) {
    if (idx >= MAX_LIGHTS) return;
    // Packet 1: posX, posY
    { Fast::UltraPacket p; p.Set(Fast::PacketType::LightPos, idx, (int16_t)(x*10.f), (int16_t)(y*10.f)); m_stream.Push(p); }
    // Packet 2: posZ, colRG
    { Fast::UltraPacket p; uint8_t pr=(uint8_t)(r*255.f), pg=(uint8_t)(g*255.f);
      p.Set(Fast::PacketType::LightPosZColor, idx, (int16_t)(z*10.f), (int16_t)(pr|(pg<<8))); m_stream.Push(p); }
    // Packet 3: colB, intensity (flag bit 23 set)
    { Fast::UltraPacket p; uint8_t pb=(uint8_t)(b*255.f), pi=(uint8_t)(intensity*63.75f);
      p.Set(Fast::PacketType::LightPosZColor, idx|(1u<<23), (int16_t)(pb|(pi<<8)), 0); m_stream.Push(p); }
}

void MegaLightRenderer::ProcessPackets(const float* camPos) {
    uint32_t wb = m_writeBuf;
    GPULightData* dst = m_mapped[wb];
    if (!dst) return;

    // Copy current read buffer to write buffer as base
    GPULightData* src = m_mapped[m_readBuf];
    if (src) memcpy(dst, src, sizeof(GPULightData) * m_lightCount.load(std::memory_order_relaxed));

    // Drain FDA ring buffer — zero-copy direct write to persistent SSBO
    Fast::UltraPacket batch[256];
    uint32_t n = m_stream.PopBatch(batch, 256);
    for (uint32_t i = 0; i < n; ++i) {
        const auto& p = batch[i];
        uint32_t idx = p.GetMetadata() & 0x7FFFFFu;
        if (idx >= MAX_LIGHTS) continue;
        GPULightData& g = dst[idx];
        if (p.GetType() == Fast::PacketType::LightPos) {
            g.posX = p.dataA * 0.1f;
            g.posY = p.dataB * 0.1f;
        } else if (p.GetType() == Fast::PacketType::LightPosZColor) {
            bool isColor = (p.GetMetadata() >> 23) & 1;
            if (!isColor) {
                g.posZ = p.dataA * 0.1f;
                uint16_t rg = (uint16_t)p.dataB;
                g.colR = (rg & 0xFF) / 255.f;
                g.colG = ((rg >> 8) & 0xFF) / 255.f;
            } else {
                uint16_t bi = (uint16_t)p.dataA;
                g.colB      = (bi & 0xFF) / 255.f;
                g.intensity = ((bi >> 8) & 0xFF) / 63.75f;
            }
        }
    }

    // Swap: write buf is now the fresh data
    m_readBuf  = wb;
    m_writeBuf = 1 - wb;

    // Sort and pack MAX_VISIBLE closest lights into front of next write buf
    SortVisibleLights(camPos);
}

void MegaLightRenderer::SortVisibleLights(const float* camPos) {
    uint32_t total = m_lightCount.load(std::memory_order_relaxed);
    if (total == 0) { m_visibleCount.store(0, std::memory_order_relaxed); return; }

    GPULightData* src = m_mapped[m_readBuf];
    if (!src) return;

    float cx = camPos ? camPos[0] : 0.f;
    float cy = camPos ? camPos[1] : 0.f;
    float cz = camPos ? camPos[2] : 0.f;

    // Build distance array
    static float    dists[MAX_LIGHTS];
    static uint32_t order[MAX_LIGHTS];
    for (uint32_t i = 0; i < total; ++i) {
        float dx = src[i].posX - cx, dy = src[i].posY - cy, dz = src[i].posZ - cz;
        dists[i] = dx*dx + dy*dy + dz*dz;
        order[i] = i;
    }

    // Partial selection sort for MAX_VISIBLE closest
    uint32_t vis = (total < MAX_VISIBLE) ? total : MAX_VISIBLE;
    for (uint32_t i = 0; i < vis; ++i) {
        uint32_t best = i;
        for (uint32_t j = i+1; j < total; ++j)
            if (dists[order[j]] < dists[order[best]]) best = j;
        uint32_t tmp = order[i]; order[i] = order[best]; order[best] = tmp;
    }

    // Pack visible lights into write buffer front
    GPULightData* wDst = m_mapped[m_writeBuf];
    if (wDst) {
        for (uint32_t i = 0; i < vis; ++i) wDst[i] = src[order[i]];
        m_readBuf  = m_writeBuf;
        m_writeBuf = 1 - m_writeBuf;
    }

    m_visibleCount.store(vis, std::memory_order_release);
}

} // namespace SecretEngine
