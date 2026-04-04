@echo off
REM Shader Compilation Script for Secret Engine
REM Compiles all GLSL shaders to SPIR-V format

echo Compiling shaders...

REM Mega Geometry Shaders
glslc plugins/VulkanRenderer/shaders/mega_geometry.vert -o android/app/src/main/assets/shaders/mega_geometry_vert.spv
glslc plugins/VulkanRenderer/shaders/mega_geometry.frag -o android/app/src/main/assets/shaders/mega_geometry_frag.spv

REM Basic 3D Shaders
glslc plugins/VulkanRenderer/shaders/basic3d.vert -o android/app/src/main/assets/shaders/basic3d_vert.spv
glslc plugins/VulkanRenderer/shaders/basic3d.frag -o android/app/src/main/assets/shaders/basic3d_frag.spv

REM Compute Shaders
glslc plugins/VulkanRenderer/shaders/cull.comp -o android/app/src/main/assets/shaders/cull_comp.spv

REM Simple 2D Shaders
glslc plugins/VulkanRenderer/shaders/simple2d.vert -o android/app/src/main/assets/shaders/simple2d_vert.spv
glslc plugins/VulkanRenderer/shaders/simple2d.frag -o android/app/src/main/assets/shaders/simple2d_frag.spv

REM Triangle Shaders
glslc plugins/VulkanRenderer/shaders/triangle.vert -o android/app/src/main/assets/shaders/triangle_vert.spv
glslc plugins/VulkanRenderer/shaders/triangle.frag -o android/app/src/main/assets/shaders/triangle_frag.spv

REM Particle Shaders (Cyberpunk City)
glslc -fshader-stage=vertex plugins/VulkanRenderer/shaders/particle_vert.glsl -o android/app/src/main/assets/shaders/particle_vert.spv
glslc -fshader-stage=fragment plugins/VulkanRenderer/shaders/particle_frag.glsl -o android/app/src/main/assets/shaders/particle_frag.spv

echo Shader compilation complete!
