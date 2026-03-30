// ============================================================================
// TEXTURE MANAGER - GPU-OPTIMIZED ASTC COMPRESSION
// High-performance texture streaming with bindless textures
// ============================================================================

#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>
#include <atomic>

namespace SecretEngine {
    class ICore;
    class IAssetProvider;
}

class VulkanDevice;

namespace SecretEngine::Textures {

// ============================================================================
// ASTC BLOCK FORMATS (Hardware-accelerated on mobile & modern GPUs)
// ============================================================================
enum class ASTCFormat {
    ASTC_4x4_UNORM = 0,   // Highest quality, 8bpp
    ASTC_5x5_UNORM,       // 5.12bpp
    ASTC_6x6_UNORM,       // 3.56bpp
    ASTC_8x8_UNORM,       // 2bpp - Best balance
    ASTC_10x10_UNORM,     // 1.28bpp
    ASTC_12x12_UNORM,     // Lowest quality, 0.89bpp
    
    // SRGB variants
    ASTC_4x4_SRGB,
    ASTC_8x8_SRGB,
    
    // Fallback formats
    BC7_UNORM,            // Desktop fallback
    RGBA8_UNORM,          // Uncompressed fallback
    
    AUTO                  // Auto-detect best format
};

// ============================================================================
// TEXTURE SLOT - GPU Bindless Resource
// ============================================================================
struct TextureSlot {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    VkFormat format = VK_FORMAT_UNDEFINED;
    
    uint64_t vramBytes = 0;
    bool isResident = false;  // GPU streaming state
    
    // Atlas support
    struct AtlasRegion {
        float u0, v0, u1, v1;  // Normalized UV coordinates
    };
    std::vector<AtlasRegion> atlasRegions;  // Sub-textures in atlas
};

// ============================================================================
// TEXTURE MANAGER - Centralized GPU Texture System
// ============================================================================
class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager();
    
    // === INITIALIZATION ===
    bool Initialize(VulkanDevice* device, SecretEngine::ICore* core);
    void Shutdown();
    
    // === TEXTURE LOADING (Auto-detects format) ===
    // Loads PNG and converts to optimal GPU format (ASTC preferred)
    uint32_t LoadTexture(const char* path, ASTCFormat preferredFormat = ASTCFormat::AUTO);
    
    // Load pre-compressed ASTC file (fastest)
    uint32_t LoadASTCTexture(const char* path, ASTCFormat format);
    
    // === TEXTURE ATLAS (Batch multiple textures into one) ===
    struct AtlasTextureInfo {
        const char* path;
        uint32_t slotIndex;  // Output: assigned texture slot in atlas
    };
    uint32_t CreateAtlas(AtlasTextureInfo* textures, uint32_t count, uint32_t atlasSize = 4096);
    
    // === BINDLESS DESCRIPTOR MANAGEMENT ===
    VkDescriptorSet GetTextureDescriptorSet() const { return m_descriptorSet; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
    
    // === STREAMING (Async GPU upload) ===
    void StreamTexture(uint32_t textureID);
    void EvictTexture(uint32_t textureID);
    
    // === ACCESSORS ===
    const TextureSlot* GetTexture(uint32_t id) const;
    uint32_t GetTextureCount() const { return static_cast<uint32_t>(m_textures.size()); }
    
    // === STATS ===
    uint64_t GetVRAMUsage() const { return m_vramAllocated.load(); }
    
    // === MATERIAL SUPPORT ===
    struct Material {
        uint32_t albedoTexture;
        uint32_t normalTexture;
        uint32_t metallicRoughnessTexture;
        float albedoFactor[4];
        float metallicFactor;
        float roughnessFactor;
    };
    uint32_t CreateMaterial(const Material& mat);
    const Material* GetMaterial(uint32_t id) const;
    
private:
    // === INTERNAL LOADING ===
    bool LoadPNGToASTC(const char* path, std::vector<uint8_t>& outData, 
                       uint32_t& width, uint32_t& height, ASTCFormat& format);
    uint32_t CreateTextureFromMemory(const void* data, uint32_t width, uint32_t height, 
                                     VkFormat format, uint32_t mipLevels = 1);
    
    // === GPU RESOURCE CREATION ===
    bool CreateImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels,
                    VkImage& image, VkDeviceMemory& memory);
    bool CreateImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageView& view);
    bool CreateSampler(VkSampler& sampler, uint32_t mipLevels);
    
    // === DESCRIPTOR SET MANAGEMENT ===
    bool CreateDescriptorSet();
    void UpdateDescriptorSet(uint32_t textureID);
    
    // === COMPRESSION HELPERS ===
    VkFormat ASTCFormatToVkFormat(ASTCFormat format) const;
    ASTCFormat SelectBestFormat() const;  // Auto-detect based on GPU capabilities
    bool IsFormatSupported(VkFormat format) const;
    
    // === STAGING ===
    bool UploadTextureData(VkImage image, const void* data, uint32_t width, uint32_t height, 
                          VkFormat format, uint32_t mipLevels);
    bool GenerateMipmaps(VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format);
    
    VulkanDevice* m_device = nullptr;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    SecretEngine::ICore* m_core = nullptr;
    
    // === TEXTURE POOL ===
    std::vector<TextureSlot> m_textures;
    std::map<std::string, uint32_t> m_textureCache;  // Path -> Texture ID
    
    // === BINDLESS DESCRIPTORS (Up to 16,384 textures) ===
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    static constexpr uint32_t MAX_TEXTURES = 16384;
    
    // === MATERIALS ===
    std::vector<Material> m_materials;
    
    // === STATS ===
    std::atomic<uint64_t> m_vramAllocated{0};
    
    // === STAGING BUFFER (For uploads) ===
    VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;
    void* m_stagingMapped = nullptr;
    static constexpr uint64_t STAGING_SIZE = 64 * 1024 * 1024;  // 64MB
    
    // === COMMAND POOL (For transfers) ===
    VkCommandPool m_transferPool = VK_NULL_HANDLE;
};

} // namespace SecretEngine::Textures
