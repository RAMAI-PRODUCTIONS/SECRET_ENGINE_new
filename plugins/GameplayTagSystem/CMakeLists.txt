# SecretEngine
# Module: GameplayTagSystem
# Responsibility: Unreal-like Gameplay Tag System + Gameplay Ability System
# Dependencies: Core, nlohmann_json

add_library(GameplayTagSystem STATIC
    src/GameplayTags.h
    src/GameplayAbilitySystem.h
    src/GameplayDataTable.cpp
    src/GameplayTagPlugin.h
    src/GameplayTagPlugin.cpp
)

target_include_directories(GameplayTagSystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(GameplayTagSystem
    PUBLIC
    SecretEngine_Core
    nlohmann_json::nlohmann_json
)

set_target_properties(GameplayTagSystem PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
