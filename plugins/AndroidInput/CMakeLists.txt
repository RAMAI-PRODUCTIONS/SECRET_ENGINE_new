# Android Input Plugin
if(ANDROID)
    add_library(AndroidInput STATIC
        src/InputPlugin.cpp
        src/InputPlugin.h
        src/UIConfig.cpp
        src/UIConfig.h
    )
else()
    add_library(AndroidInput SHARED
        src/InputPlugin.cpp
        src/InputPlugin.h
        src/UIConfig.cpp
        src/UIConfig.h
    )
endif()

target_include_directories(AndroidInput PRIVATE src)
target_link_libraries(AndroidInput PRIVATE SecretEngine_Core)

# Force fno-rtti/exceptions
if(NOT MSVC)
    target_compile_options(AndroidInput PRIVATE -fno-exceptions -fno-rtti)
endif()
