cmake_minimum_required(VERSION 3.10)
project(TextureSystem)

set(CMAKE_CXX_STANDARD 17)

add_library(TextureSystem SHARED
    src/TexturePlugin.cpp
)

target_include_directories(TextureSystem PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    src
)

target_link_libraries(TextureSystem PRIVATE
    SecretEngine_Core
)

set_target_properties(TextureSystem PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)
