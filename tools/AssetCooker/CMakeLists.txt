add_executable(AssetCooker src/main.cpp)

target_link_libraries(AssetCooker PRIVATE tinygltf)

# On Windows, we need to set some flags for tinygltf
if(WIN32)
    target_compile_definitions(AssetCooker PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
