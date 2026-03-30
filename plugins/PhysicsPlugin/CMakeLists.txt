# PhysicsPlugin
add_library(PhysicsPlugin STATIC
    src/PhysicsPlugin.h
    src/PhysicsPlugin.cpp
)

target_link_libraries(PhysicsPlugin
    PUBLIC
        SecretEngine_Core
)

target_include_directories(PhysicsPlugin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(PhysicsPlugin PROPERTIES
    FOLDER "Plugins"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
