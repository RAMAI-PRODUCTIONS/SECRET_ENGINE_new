# FPSUIPlugin
add_library(FPSUIPlugin STATIC
    src/FPSUIPlugin.h
    src/FPSUIPlugin.cpp
)

target_link_libraries(FPSUIPlugin
    PUBLIC
        SecretEngine_Core
        LevelSystem
)

target_include_directories(FPSUIPlugin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(FPSUIPlugin PROPERTIES
    FOLDER "Plugins"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
