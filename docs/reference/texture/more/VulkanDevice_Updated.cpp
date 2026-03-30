// ============================================================================
// VULKAN DEVICE - UPDATED WITH DESCRIPTOR INDEXING SUPPORT
// Critical for bindless texture arrays
// ============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "VulkanDevice.h"
#include <vulkan/vulkan.h>

#if defined(SE_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
#elif defined(SE_PLATFORM_ANDROID)
    #include <vulkan/vulkan_android.h>
#endif

#include <cstdio>
#include <vector>
#include <SecretEngine/ILogger.h>

VulkanDevice::VulkanDevice(SecretEngine::ICore* core) : m_core(core) {}

VulkanDevice::~VulkanDevice() { 
    Shutdown(); 
}

bool VulkanDevice::Initialize() {
    m_core->GetLogger()->LogInfo("VulkanDevice", "Starting Vulkan Initialization...");

    if (!CreateInstance()) {
        m_core->GetLogger()->LogError("VulkanDevice", "CRITICAL: Failed to create Vulkan Instance.");
        return false;
    }

    if (!PickPhysicalDevice()) {
        m_core->GetLogger()->LogError("VulkanDevice", "CRITICAL: Failed to find a suitable GPU.");
        return false;
    }

    if (!CreateLogicalDevice()) {
        m_core->GetLogger()->LogError("VulkanDevice", "CRITICAL: Failed to create Logical Device.");
        return false;
    }

    m_core->GetLogger()->LogInfo("VulkanDevice", "Vulkan Hardware fully initialized.");
    return true;
}

bool VulkanDevice::CreateInstance() {
    std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };

#if defined(SE_PLATFORM_WINDOWS)
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(SE_PLATFORM_ANDROID)
    extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
    
    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "SecretEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "SecretEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;  // Need 1.2 for descriptor indexing

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        char buf[128];
        sprintf(buf, "vkCreateInstance failed with error code: %d", (int)result);
        m_core->GetLogger()->LogError("VulkanDevice", buf);
        return false;
    }

    return true;
}

bool VulkanDevice::CreateSurface(void* nativeWindow) {
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;

    if (!nativeWindow) {
        m_core->GetLogger()->LogError("VulkanDevice", "Cannot create surface: Native Window handle is NULL.");
        return false;
    }

#if defined(SE_PLATFORM_WINDOWS)
    VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    createInfo.hwnd = (HWND)nativeWindow;
    createInfo.hinstance = GetModuleHandle(nullptr);
    result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);

#elif defined(SE_PLATFORM_ANDROID)
    VkAndroidSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = (ANativeWindow*)nativeWindow;
    
    result = vkCreateAndroidSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
#endif

    if (result != VK_SUCCESS) {
        m_core->GetLogger()->LogError("VulkanDevice", "Failed to create Window Surface.");
        return false;
    }
    return true;
}

bool VulkanDevice::PickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    
    if (count == 0) {
        m_core->GetLogger()->LogError("VulkanDevice", "No Vulkan-compatible GPUs found on this system.");
        return false;
    }

    VkPhysicalDevice devices[8];
    vkEnumeratePhysicalDevices(m_instance, &count, devices);
    m_physicalDevice = devices[0]; 

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    m_core->GetLogger()->LogInfo("VulkanDevice", props.deviceName);

    return true;
}

bool VulkanDevice::CreateLogicalDevice() {
    // ============================================================================
    // CRITICAL: Enable Descriptor Indexing Features for Bindless Textures
    // ============================================================================
    
    // 1. Query descriptor indexing support
    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
    };
    
    VkPhysicalDeviceFeatures2 supportedFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    supportedFeatures.pNext = &indexingFeatures;
    
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supportedFeatures);
    
    // 2. Log support status
    if (indexingFeatures.shaderSampledImageArrayNonUniformIndexing) {
        m_core->GetLogger()->LogInfo("VulkanDevice", "✓ Bindless textures supported (descriptor indexing)");
    } else {
        m_core->GetLogger()->LogWarning("VulkanDevice", "⚠ Bindless textures NOT supported on this GPU");
    }
    
    // 3. Enable required features
    VkPhysicalDeviceDescriptorIndexingFeatures enabledIndexingFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
    };
    enabledIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    enabledIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
    enabledIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    enabledIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
    enabledIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures2.pNext = &enabledIndexingFeatures;
    
    // 4. Queue creation
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = 0; 
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    // 5. Device extensions (MUST include descriptor indexing extension)
    const char* deviceExtensions[] = { 
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME  // CRITICAL for bindless
    };
    
    // 6. Device creation with features
    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.pNext = &deviceFeatures2;  // Chain the features
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        char buf[128];
        sprintf(buf, "vkCreateDevice failed with error code: %d", (int)result);
        m_core->GetLogger()->LogError("VulkanDevice", buf);
        return false;
    }

    vkGetDeviceQueue(m_device, 0, 0, &m_graphicsQueue);
    return true;
}

void VulkanDevice::Shutdown() {
    if (m_surface) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    if (m_device) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_instance) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}
