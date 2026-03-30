# SecretEngine
# Module: LevelSystem
# Responsibility: Unreal-like Level Management System
# Dependencies: Core, nlohmann_json

add_library(LevelSystem STATIC
    src/LevelTypes.h
    src/LevelManager.h
    src/LevelManager.cpp
    src/LevelLoader.h
    src/LevelLoader.cpp
    src/LevelSystemPlugin.h
    src/LevelSystemPlugin.cpp
)

target_include_directories(LevelSystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(LevelSystem
    PUBLIC
    SecretEngine_Core
    nlohmann_json::nlohmann_json
)

set_target_properties(LevelSystem PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
