cmake_minimum_required(VERSION 3.18)
project(ProceduralCityPlugin)

add_library(ProceduralCityPlugin SHARED
    src/ProceduralCityPlugin.cpp
)

target_include_directories(ProceduralCityPlugin PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(ProceduralCityPlugin PRIVATE core)

set_target_properties(ProceduralCityPlugin PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
