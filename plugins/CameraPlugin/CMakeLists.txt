cmake_minimum_required(VERSION 3.18)
project(CameraPlugin)

# Header-only library
add_library(CameraPlugin INTERFACE)

target_include_directories(CameraPlugin INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/core/include
)
