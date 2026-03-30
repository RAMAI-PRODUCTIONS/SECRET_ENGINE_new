#pragma once
#include <vulkan/vulkan.h>
#include <SecretEngine/ICore.h>

class VulkanDevice {
public:
    VulkanDevice(SecretEngine::ICore* core);
    ~VulkanDevice();

    bool Initialize();
    bool CreateSurface(void* nativeWindow); // Exposed
    void Shutdown();

    VkDevice GetDevice() const { return m_device; }
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkSurfaceKHR GetSurface() const { return m_surface; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    

private:
    bool CreateInstance();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();

    SecretEngine::ICore* m_core;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE; // Added
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
};