#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "VulkanDevice.h"
#include <vulkan/vulkan.h> // MUST BE FIRST

// 1. Platform Specific Headers
#if defined(SE_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
#elif defined(SE_PLATFORM_ANDROID)
    #include <vulkan/vulkan_android.h>
#endif

// 2. Standard Libraries
#include <cstdio>
#include <vector>

// 3. Vulkan Headers
// Already included at top

// 4. Engine Headers
#include <SecretEngine/ILogger.h>
#include "VulkanDevice.h"

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
    // Setup platform-specific extensions
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
    appInfo.apiVersion = VK_API_VERSION_1_2;

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

    // Just pick the first available device for now
    VkPhysicalDevice devices[8];
    vkEnumeratePhysicalDevices(m_instance, &count, devices);
    m_physicalDevice = devices[0]; 

    // Log the name of the GPU we found
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    m_core->GetLogger()->LogInfo("VulkanDevice", props.deviceName);

    return true;
}

bool VulkanDevice::CreateLogicalDevice() {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = 0; 
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    // Check which extensions are available
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    
    // Build list of extensions to enable
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME); // Always required
    
    // Check if descriptor indexing is available (for bindless textures)
    bool hasDescriptorIndexing = false;
    for (const auto& ext : availableExtensions) {
        if (strcmp(ext.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0) {
            hasDescriptorIndexing = true;
            deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            m_core->GetLogger()->LogInfo("VulkanDevice", "✓ Descriptor indexing extension available");
            break;
        }
    }
    
    if (!hasDescriptorIndexing) {
        m_core->GetLogger()->LogWarning("VulkanDevice", "Descriptor indexing not available - bindless textures disabled");
    }
    
    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Query and enable descriptor indexing features when available.
    //
    // IMPORTANT: We must not hard-link against vkGetPhysicalDeviceFeatures2 on platforms where
    // it's not exported (some Android loaders), so we resolve it dynamically.
    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
    };
    VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    
    // Only chain descriptor indexing features if the extension is available
    if (hasDescriptorIndexing) {
        features2.pNext = &descriptorIndexingFeatures;
    }

    auto fpGetPhysicalDeviceFeatures2 =
        reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
            vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceFeatures2"));

    if (fpGetPhysicalDeviceFeatures2 && hasDescriptorIndexing) {
        // Safe path: loader exports vkGetPhysicalDeviceFeatures2 (Vulkan 1.1+ or extension)
        fpGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);
        createInfo.pNext = &features2;
    } else {
        // Fallback: descriptor indexing querying not available; continue without chaining
        // extended features. Bindless textures may be limited on this platform.
        createInfo.pNext = nullptr;
        if (!hasDescriptorIndexing) {
            m_core->GetLogger()->LogWarning(
                "VulkanDevice",
                "Descriptor indexing not supported; bindless textures disabled");
        } else {
            m_core->GetLogger()->LogWarning(
                "VulkanDevice",
                "vkGetPhysicalDeviceFeatures2 not available; running without explicit descriptor indexing features");
        }
    }

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