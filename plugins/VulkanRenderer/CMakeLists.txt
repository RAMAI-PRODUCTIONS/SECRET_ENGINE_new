# SecretEngine - VulkanRenderer Plugin CMake
if(ANDROID)
    add_library(VulkanRenderer STATIC
        src/RendererPlugin.cpp
        src/VulkanDevice.cpp
        src/Swapchain.cpp
        src/Window.cpp
        src/Pipeline3D.cpp
        src/MegaGeometryRenderer.cpp
        src/VulkanHelpers.cpp
        src/TextureManager.cpp
        src/MeshRenderingSystem.cpp
    )
else()
    add_library(VulkanRenderer SHARED
        src/RendererPlugin.cpp
        src/VulkanDevice.cpp
        src/Swapchain.cpp
        src/Window.cpp
        src/Pipeline3D.cpp
        src/MegaGeometryRenderer.cpp
        src/VulkanHelpers.cpp
        src/TextureManager.cpp
        src/MeshRenderingSystem.cpp
    )
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(VulkanRenderer PRIVATE SecretEngine_Core Vulkan::Vulkan tinygltf nlohmann_json::nlohmann_json)

if(ANDROID)
    target_include_directories(VulkanRenderer PRIVATE "${ANDROID_NDK}/sources/android/native_app_glue")
endif()

# Force the DLL to build inside the tests folder for easy discovery
set_target_properties(VulkanRenderer PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests/Debug/plugins/VulkanRenderer"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/tests/Debug/plugins/VulkanRenderer"
)

# Automate manifest deployment
configure_file(
    plugin_manifest.json
    ${CMAKE_BINARY_DIR}/tests/Debug/plugins/VulkanRenderer/plugin_manifest.json
    COPYONLY
)