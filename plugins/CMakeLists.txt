add_subdirectory(VulkanRenderer)
add_subdirectory(CameraPlugin)
add_subdirectory(DebugPlugin)

# NEW: Core System Plugins (Phase 2)
add_subdirectory(LightingSystem)
add_subdirectory(MaterialSystem)
add_subdirectory(TextureSystem)
add_subdirectory(ShadowSystem)

# NEW: FPS Game Plugins
add_subdirectory(PhysicsPlugin)
add_subdirectory(GameplayTagSystem)
add_subdirectory(LevelSystem)
add_subdirectory(FPSGameLogic)
add_subdirectory(FPSUIPlugin)

if(ANDROID)
    add_subdirectory(AndroidInput)
    add_subdirectory(GameLogic)
else()
    add_subdirectory(WindowsInput)
    add_subdirectory(GameLogic)
endif()