#include <vulkan/vulkan.h>
#include <SecretEngine/ICore.h> 
#include <vector>

class VulkanDevice;

class Swapchain {
public:
    Swapchain(VulkanDevice* device, SecretEngine::ICore* core);
    ~Swapchain();

    bool Create(VkSurfaceKHR surface, uint32_t width, uint32_t height);
    void Cleanup();

    VkFormat GetFormat() const { return m_imageFormat; }
    VkExtent2D GetExtent() const { return m_extent; }
    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
    const std::vector<VkImageView>& GetImageViews() const { return m_imageViews; }

private:
    void CreateImageViews();

    VulkanDevice* m_device;
    SecretEngine::ICore* m_core;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_imageFormat;
    VkExtent2D m_extent;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
};