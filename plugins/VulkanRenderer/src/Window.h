#pragma once
#if defined(SE_PLATFORM_WINDOWS)
    #include <windows.h>
#elif defined(SE_PLATFORM_ANDROID)
    #include <android/native_window.h>
#endif
#include <vulkan/vulkan.h>

class Window {
public:
    bool Create(int width, int height, const char* title);
    void PollEvents();
    bool ShouldClose() const { return m_shouldClose; }
    VkSurfaceKHR CreateSurface(VkInstance instance);

private:
#if defined(SE_PLATFORM_WINDOWS)
    HWND m_hwnd = nullptr;
#endif
    bool m_shouldClose = false;
};