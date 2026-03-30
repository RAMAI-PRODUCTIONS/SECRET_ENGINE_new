#include "Swapchain.h"
#include <string>
#include "VulkanDevice.h"
#include <SecretEngine/ILogger.h>

Swapchain::Swapchain(VulkanDevice* device, SecretEngine::ICore* core) : m_device(device), m_core(core) {}
Swapchain::~Swapchain() { Cleanup(); }

bool Swapchain::Create(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    // 1. Query Surface Capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->GetPhysicalDevice(), surface, &capabilities);

    VkExtent2D actualExtent = capabilities.currentExtent;
    if (actualExtent.width == UINT32_MAX) {
        actualExtent.width = width;
        actualExtent.height = height;
    }
    m_extent = actualExtent;
    
    // 2. Query supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->GetPhysicalDevice(), surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device->GetPhysicalDevice(), surface, &formatCount, surfaceFormats.data());
    
    // Select the best format (Prefer R8G8B8A8_UNORM on Android)
    VkSurfaceFormatKHR selectedFormat = surfaceFormats[0]; 
    for (const auto& format : surfaceFormats) {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selectedFormat = format;
            break;
        }
    }
    
    m_core->GetLogger()->LogInfo("Swapchain", ("Selected surface format: " + std::to_string(selectedFormat.format)).c_str());

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = 3; 
    if (createInfo.minImageCount < capabilities.minImageCount) createInfo.minImageCount = capabilities.minImageCount;
    if (capabilities.maxImageCount > 0 && createInfo.minImageCount > capabilities.maxImageCount) createInfo.minImageCount = capabilities.maxImageCount;

    createInfo.imageFormat = selectedFormat.format;
    createInfo.imageColorSpace = selectedFormat.colorSpace;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = capabilities.currentTransform;
    
    // Prefer OPAQUE or INHERIT for Android
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else {
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device->GetDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        m_core->GetLogger()->LogError("Swapchain", "Failed to create swapchain!");
        return false;
    }

    // 2. Retrieve Images
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device->GetDevice(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device->GetDevice(), m_swapchain, &imageCount, m_images.data());

    m_imageFormat = createInfo.imageFormat;

    CreateImageViews();
    return true;
}

void Swapchain::CreateImageViews() {
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); i++) {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = m_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device->GetDevice(), &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS) {
            m_core->GetLogger()->LogError("Swapchain", "Failed to create image views!");
        }
    }
}

void Swapchain::Cleanup() {
    for (auto imageView : m_imageViews) {
        vkDestroyImageView(m_device->GetDevice(), imageView, nullptr);
    }
    m_imageViews.clear();
    
    if (m_swapchain) vkDestroySwapchainKHR(m_device->GetDevice(), m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}