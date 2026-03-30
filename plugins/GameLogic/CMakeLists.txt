# Game Logic Plugin
if(ANDROID)
    add_library(GameLogic STATIC
        src/LogicPlugin.cpp
        src/LogicPlugin.h
    )
else()
    add_library(GameLogic SHARED
        src/LogicPlugin.cpp
        src/LogicPlugin.h
    )
endif()

target_include_directories(GameLogic PRIVATE src)
target_link_libraries(GameLogic PRIVATE SecretEngine_Core)

if(NOT MSVC)
    target_compile_options(GameLogic PRIVATE -fno-exceptions -fno-rtti)
endif()
