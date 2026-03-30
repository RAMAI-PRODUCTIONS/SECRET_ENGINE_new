cmake_minimum_required(VERSION 3.10)
project(LightingSystem)

set(CMAKE_CXX_STANDARD 17)

add_library(LightingSystem SHARED
    src/LightingPlugin.cpp
    src/LightManager.cpp
)

target_include_directories(LightingSystem PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    src
)

target_link_libraries(LightingSystem PRIVATE
    SecretEngine_Core
)

set_target_properties(LightingSystem PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)
