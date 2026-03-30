#include "VulkanHelpers.h"
#include <cstring>
#include <atomic>

// === VRAM TRACKING (Thread-Safe) ===
namespace {
    std::atomic<uint64_t> g_vramAllocated{0};
}

namespace SecretEngine::Vulkan {

bool Helpers::CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                         VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                         VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    
    // === TRACK VRAM ALLOCATION ===
    g_vramAllocated.fetch_add(memRequirements.size, std::memory_order_relaxed);
    
    return true;
}

VkShaderModule Helpers::CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

uint32_t Helpers::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                               VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

void Helpers::MapAndCopy(VkDevice device, VkDeviceMemory memory, VkDeviceSize size, const void* data) {
    void* mappedData;
    vkMapMemory(device, memory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, size);
    vkUnmapMemory(device, memory);
}

// === VRAM STATS API ===
uint64_t Helpers::GetVRAMAllocated() {
    return g_vramAllocated.load(std::memory_order_relaxed);
}

} // namespace SecretEngine::Vulkan
