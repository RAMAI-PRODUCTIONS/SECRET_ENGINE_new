cmake_minimum_required(VERSION 3.10)
project(MaterialSystem)

set(CMAKE_CXX_STANDARD 17)

add_library(MaterialSystem SHARED
    src/MaterialPlugin.cpp
)

target_include_directories(MaterialSystem PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    src
)

target_link_libraries(MaterialSystem PRIVATE
    SecretEngine_Core
)

set_target_properties(MaterialSystem PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)
