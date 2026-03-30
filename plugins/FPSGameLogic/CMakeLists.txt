# FPSGameLogic Plugin
add_library(FPSGameLogic STATIC
    src/FPSGamePlugin.h
    src/FPSGamePlugin.cpp
    src/FPSComponents.h
    src/FPSFastData.h
    src/FPSSystems.cpp
)

target_link_libraries(FPSGameLogic
    PUBLIC
        SecretEngine_Core
        GameplayTagSystem
)

target_include_directories(FPSGameLogic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(FPSGameLogic PROPERTIES
    FOLDER "Plugins"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
