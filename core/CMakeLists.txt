# SecretEngine - Core CMake
# Responsibility: Core static library definition

add_library(SecretEngine_Core STATIC
    src/Core.cpp
    src/SystemAllocator.cpp
    src/JobSystem.cpp
    src/Logger.cpp
    src/PluginManager.cpp
    src/Entity.cpp
    src/World.cpp
    src/AssetProvider.cpp
)

target_include_directories(SecretEngine_Core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(ANDROID)
    target_include_directories(SecretEngine_Core PRIVATE "${ANDROID_NDK}/sources/android/native_app_glue")
endif()

# Iron Rules: No Exceptions, No RTTI
if(MSVC)
    # Define _HAS_EXCEPTIONS=0 to tell the STL we are exception-free
    target_compile_definitions(SecretEngine_Core PUBLIC _HAS_EXCEPTIONS=0)
    
    # Remove the default /EH flags to stop the D9025 override warning
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    
    target_compile_options(SecretEngine_Core PRIVATE /EHs- /GR-)
else()
    target_compile_options(SecretEngine_Core PRIVATE -fno-exceptions -fno-rtti)
endif()

target_link_libraries(SecretEngine_Core PRIVATE nlohmann_json::nlohmann_json)