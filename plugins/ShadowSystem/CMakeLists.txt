cmake_minimum_required(VERSION 3.10)
project(ShadowSystem)

set(CMAKE_CXX_STANDARD 17)

add_library(ShadowSystem SHARED
    src/ShadowPlugin.cpp
)

target_include_directories(ShadowSystem PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    src
)

target_link_libraries(ShadowSystem PRIVATE
    SecretEngine_Core
)

set_target_properties(ShadowSystem PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)
