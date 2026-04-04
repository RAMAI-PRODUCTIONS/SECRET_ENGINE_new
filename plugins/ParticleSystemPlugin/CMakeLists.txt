cmake_minimum_required(VERSION 3.18)
project(ParticleSystemPlugin)

add_library(ParticleSystemPlugin SHARED
    src/ParticleSystemPlugin.cpp
)

target_include_directories(ParticleSystemPlugin PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(ParticleSystemPlugin PRIVATE SecretEngine_Core)

set_target_properties(ParticleSystemPlugin PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
