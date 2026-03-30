# SecretEngine
# Module: tests
# Responsibility: Test suite build configuration

add_executable(PluginLoadTest PluginLoadTest.cpp)
target_link_libraries(PluginLoadTest PRIVATE SecretEngine_Core)