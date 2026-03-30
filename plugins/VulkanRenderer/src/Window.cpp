#include "Window.h"

#if defined(SE_PLATFORM_WINDOWS)
#include <vulkan/vulkan_win32.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool Window::Create(int width, int height, const char* title) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "SecretEngineWin";
    RegisterClass(&wc);

    // Added WS_VISIBLE to force the window to show immediately
    m_hwnd = CreateWindowEx(0, wc.lpszClassName, title, 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, 
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!m_hwnd) return false;

    // Ensure it comes to the front
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
    
    return true;
}

void Window::PollEvents() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) m_shouldClose = true;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance) {
    VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    createInfo.hwnd = m_hwnd;
    createInfo.hinstance = GetModuleHandle(nullptr);
    VkSurfaceKHR surface;
    vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
    return surface;
}
#elif defined(SE_PLATFORM_ANDROID)
// Android Dummy Implementation - Window is managed by AndroidMain
bool Window::Create(int width, int height, const char* title) { return true; }
void Window::PollEvents() {}
VkSurfaceKHR Window::CreateSurface(VkInstance instance) { return VK_NULL_HANDLE; }
#endif