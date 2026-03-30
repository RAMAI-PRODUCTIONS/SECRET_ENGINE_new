project(DebugPlugin)

add_library(DebugPlugin STATIC
    src/DebugPlugin.cpp
    src/DebugPlugin.h
    src/Profiler.cpp
    src/Profiler.h
)

target_include_directories(DebugPlugin PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/core/include
)
