#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

// Undefine Windows API macro that conflicts with our CreateBuffer function
#ifdef CreateBuffer
#undef CreateBuffer
#endif

namespace SecretEngine::Vulkan {

class Helpers {
public:
    static bool CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                           VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                           VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);

    static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                                 VkMemoryPropertyFlags properties);

    static void MapAndCopy(VkDevice device, VkDeviceMemory memory, VkDeviceSize size, const void* data);
    
    // VRAM tracking (thread-safe)
    static uint64_t GetVRAMAllocated();
};

} // namespace SecretEngine::Vulkan
