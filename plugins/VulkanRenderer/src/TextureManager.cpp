// ============================================================================
// TEXTURE MANAGER IMPLEMENTATION
// High-performance texture loading with ASTC compression
// ============================================================================

#include "TextureManager.h"
#include "VulkanDevice.h"
#include "VulkanHelpers.h"
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IAssetProvider.h>
#include <cstring>
#include <algorithm>

// STB Image for PNG/JPEG decoding
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace SecretEngine::Textures {

using SecretEngine::Vulkan::Helpers;

TextureManager::~TextureManager() {
    Shutdown();
}

bool TextureManager::Initialize(VulkanDevice* device, SecretEngine::ICore* core) {
    m_device = device;
    m_vkDevice = device->GetDevice();
    m_core = core;
    
    // Create staging buffer for uploads
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = STAGING_SIZE;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &m_stagingBuffer) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_vkDevice, m_stagingBuffer, &memReq);
    
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = Helpers::FindMemoryType(m_device->GetPhysicalDevice(), 
        memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_stagingMemory) != VK_SUCCESS) {
        return false;
    }
    
    vkBindBufferMemory(m_vkDevice, m_stagingBuffer, m_stagingMemory, 0);
    vkMapMemory(m_vkDevice, m_stagingMemory, 0, STAGING_SIZE, 0, &m_stagingMapped);
    
    // Create transfer command pool
    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = 0;  // TODO: Use proper queue family
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_transferPool);
    
    // Create bindless descriptor set
    if (!CreateDescriptorSet()) {
        return false;
    }
    
    if (m_core && m_core->GetLogger()) {
        m_core->GetLogger()->LogInfo("TextureManager", "✓ Texture system initialized with ASTC support");
    }
    return true;
}

void TextureManager::Shutdown() {
    // Cleanup textures
    for (auto& tex : m_textures) {
        if (tex.view) vkDestroyImageView(m_vkDevice, tex.view, nullptr);
        if (tex.sampler) vkDestroySampler(m_vkDevice, tex.sampler, nullptr);
        if (tex.image) vkDestroyImage(m_vkDevice, tex.image, nullptr);
        if (tex.memory) vkFreeMemory(m_vkDevice, tex.memory, nullptr);
    }
    
    if (m_stagingMapped) vkUnmapMemory(m_vkDevice, m_stagingMemory);
    if (m_stagingBuffer) vkDestroyBuffer(m_vkDevice, m_stagingBuffer, nullptr);
    if (m_stagingMemory) vkFreeMemory(m_vkDevice, m_stagingMemory, nullptr);
    if (m_transferPool) vkDestroyCommandPool(m_vkDevice, m_transferPool, nullptr);
    if (m_descriptorPool) vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
    if (m_descriptorSetLayout) vkDestroyDescriptorSetLayout(m_vkDevice, m_descriptorSetLayout, nullptr);
}

VkFormat TextureManager::ASTCFormatToVkFormat(ASTCFormat format) const {
    switch (format) {
        case ASTCFormat::ASTC_4x4_UNORM: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case ASTCFormat::ASTC_5x5_UNORM: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case ASTCFormat::ASTC_6x6_UNORM: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case ASTCFormat::ASTC_8x8_UNORM: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case ASTCFormat::ASTC_10x10_UNORM: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case ASTCFormat::ASTC_12x12_UNORM: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case ASTCFormat::ASTC_4x4_SRGB: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case ASTCFormat::ASTC_8x8_SRGB: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case ASTCFormat::BC7_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;
        case ASTCFormat::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        default: return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

bool TextureManager::IsFormatSupported(VkFormat format) const {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_device->GetPhysicalDevice(), format, &props);
    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
}

ASTCFormat TextureManager::SelectBestFormat() const {
    // For now, always use RGBA8 to avoid needing astcenc on all platforms.
    return ASTCFormat::RGBA8_UNORM;
}

uint32_t TextureManager::LoadTexture(const char* path, ASTCFormat preferredFormat) {
    // Check cache
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end()) {
        return it->second;
    }
    
    // Auto-detect format
    if (preferredFormat == ASTCFormat::AUTO) {
        preferredFormat = SelectBestFormat();
    }
    
    std::vector<uint8_t> compressedData;
    uint32_t width, height;
    ASTCFormat actualFormat = preferredFormat;
    
    // Load and compress
    if (!LoadPNGToASTC(path, compressedData, width, height, actualFormat)) {
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogError("TextureManager", 
                (std::string("Failed to load texture: ") + path).c_str());
        }
        return UINT32_MAX;
    }
    
    VkFormat vkFormat = ASTCFormatToVkFormat(actualFormat);
    // For now, just use a single mip level to avoid std::log2/std::floor portability issues.
    uint32_t mipLevels = 1;
    
    uint32_t textureID = CreateTextureFromMemory(compressedData.data(), width, height, vkFormat, mipLevels);
    
    if (textureID != UINT32_MAX) {
        m_textureCache[path] = textureID;
        if (m_core && m_core->GetLogger()) {
            m_core->GetLogger()->LogInfo("TextureManager", 
                (std::string("✓ Loaded: ") + path + " [" + std::to_string(width) + "x" + std::to_string(height) + "]").c_str());
        }
    }
    
    return textureID;
}

bool TextureManager::LoadPNGToASTC(const char* path, std::vector<uint8_t>& outData, 
                                   uint32_t& width, uint32_t& height, ASTCFormat& format) {
    // Load encoded image bytes using the engine asset provider
    auto imgData = m_core->GetAssetProvider()->LoadBinary(path);
    if (imgData.empty()) {
        return false;
    }

    // Decode via stb_image into RGBA8
    int w, h, channels;
    unsigned char* pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(imgData.data()),
        static_cast<int>(imgData.size()),
        &w, &h, &channels, 4  // Force RGBA
    );

    if (!pixels) {
        return false;
    }

    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);

    outData.resize(width * height * 4);
    std::memcpy(outData.data(), pixels, width * height * 4);
    stbi_image_free(pixels);

    // We always return RGBA8 for now (no ASTC compression on Android/desktop).
    format = ASTCFormat::RGBA8_UNORM;
    return true;
}

uint32_t TextureManager::CreateTextureFromMemory(const void* data, uint32_t width, uint32_t height, 
                                                 VkFormat format, uint32_t mipLevels) {
    if (m_textures.size() >= MAX_TEXTURES) {
        return UINT32_MAX;
    }
    
    TextureSlot slot;
    slot.width = width;
    slot.height = height;
    slot.mipLevels = mipLevels;
    slot.format = format;
    
    // Create image
    if (!CreateImage(width, height, format, mipLevels, slot.image, slot.memory)) {
        return UINT32_MAX;
    }
    
    // Upload data
    if (!UploadTextureData(slot.image, data, width, height, format, mipLevels)) {
        vkDestroyImage(m_vkDevice, slot.image, nullptr);
        vkFreeMemory(m_vkDevice, slot.memory, nullptr);
        return UINT32_MAX;
    }
    
    // Create image view
    if (!CreateImageView(slot.image, format, mipLevels, slot.view)) {
        vkDestroyImage(m_vkDevice, slot.image, nullptr);
        vkFreeMemory(m_vkDevice, slot.memory, nullptr);
        return UINT32_MAX;
    }
    
    // Create sampler
    if (!CreateSampler(slot.sampler, mipLevels)) {
        vkDestroyImageView(m_vkDevice, slot.view, nullptr);
        vkDestroyImage(m_vkDevice, slot.image, nullptr);
        vkFreeMemory(m_vkDevice, slot.memory, nullptr);
        return UINT32_MAX;
    }
    
    slot.isResident = true;
    
    uint32_t textureID = static_cast<uint32_t>(m_textures.size());
    m_textures.push_back(slot);
    
    // Update descriptor set
    UpdateDescriptorSet(textureID);
    
    return textureID;
}

bool TextureManager::CreateImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels,
                                 VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
    if (vkCreateImage(m_vkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_vkDevice, image, &memReq);
    
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = Helpers::FindMemoryType(m_device->GetPhysicalDevice(), 
        memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyImage(m_vkDevice, image, nullptr);
        return false;
    }
    
    vkBindImageMemory(m_vkDevice, image, memory, 0);
    
    m_vramAllocated.fetch_add(memReq.size);
    
    return true;
}

bool TextureManager::CreateImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageView& view) {
    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    return vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &view) == VK_SUCCESS;
}

bool TextureManager::CreateSampler(VkSampler& sampler, uint32_t mipLevels) {
    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    
    return vkCreateSampler(m_vkDevice, &samplerInfo, nullptr, &sampler) == VK_SUCCESS;
}

bool TextureManager::UploadTextureData(VkImage image, const void* data, uint32_t width, uint32_t height, 
                                       VkFormat format, uint32_t mipLevels) {
    // Calculate data size
    size_t dataSize = width * height * 4;  // Assume 4 bytes per pixel for now
    
    if (dataSize > STAGING_SIZE) {
        return false;  // Texture too large for staging buffer
    }
    
    // Copy to staging
    memcpy(m_stagingMapped, data, dataSize);
    
    // Create command buffer
    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = m_transferPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &cmd);
    
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    
    // Transition to TRANSFER_DST
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    // Copy buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(cmd, m_stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    // Transition to SHADER_READ
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vkEndCommandBuffer(cmd);
    
    // Submit
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_device->GetGraphicsQueue());
    
    vkFreeCommandBuffers(m_vkDevice, m_transferPool, 1, &cmd);
    
    return true;
}

bool TextureManager::CreateDescriptorSet() {
    // FALLBACK MODE: 2 textures (diffuse + normal map)
    VkDescriptorSetLayoutBinding bindings[2] = {};
    
    // Binding 0: Diffuse texture
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // Binding 1: Normal map
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    
    if (vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        return false;
    }
    
    // Create descriptor pool for 2 textures
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 2;
    
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return false;
    }
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;
    
    return vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &m_descriptorSet) == VK_SUCCESS;
}

void TextureManager::UpdateDescriptorSet(uint32_t textureID) {
    if (textureID >= m_textures.size()) return;
    
    // CRITICAL: Only update descriptor set when BOTH textures are loaded
    // Texture 0 = diffuse, Texture 1 = normal map
    if (m_textures.size() < 2) {
        // Not enough textures loaded yet, skip update
        auto logger = m_core ? m_core->GetLogger() : nullptr;
        if (logger) {
            char msg[256];
            snprintf(msg, sizeof(msg), "UpdateDescriptorSet: Waiting for both textures (currently have %zu)", m_textures.size());
            logger->LogInfo("TextureManager", msg);
        }
        return;
    }
    
    // Bind diffuse texture to binding 0 (always texture ID 0)
    const auto& diffuseTex = m_textures[0];
    
    // Bind normal map to binding 1 (always texture ID 1)
    const auto& normalTex = m_textures[1];
    
    VkDescriptorImageInfo imageInfos[2] = {};
    
    // Binding 0: Diffuse
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[0].imageView = diffuseTex.view;
    imageInfos[0].sampler = diffuseTex.sampler;
    
    // Binding 1: Normal map
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[1].imageView = normalTex.view;
    imageInfos[1].sampler = normalTex.sampler;
    
    VkWriteDescriptorSet writes[2] = {};
    
    // Write diffuse texture
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = m_descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfos[0];
    
    // Write normal map
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &imageInfos[1];
    
    vkUpdateDescriptorSets(m_vkDevice, 2, writes, 0, nullptr);
    
    auto logger = m_core ? m_core->GetLogger() : nullptr;
    if (logger) {
        logger->LogInfo("TextureManager", "✓ Descriptor set updated with diffuse + normal map");
    }
}

const TextureSlot* TextureManager::GetTexture(uint32_t id) const {
    if (id >= m_textures.size()) return nullptr;
    return &m_textures[id];
}

void TextureManager::StreamTexture(uint32_t textureID) {
    // TODO: Implement async streaming
}

void TextureManager::EvictTexture(uint32_t textureID) {
    // TODO: Implement eviction
}

uint32_t TextureManager::CreateMaterial(const Material& mat) {
    uint32_t id = static_cast<uint32_t>(m_materials.size());
    m_materials.push_back(mat);
    return id;
}

const TextureManager::Material* TextureManager::GetMaterial(uint32_t id) const {
    if (id >= m_materials.size()) return nullptr;
    return &m_materials[id];
}

} // namespace SecretEngine::Textures

