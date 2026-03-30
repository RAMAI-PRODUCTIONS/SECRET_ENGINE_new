@echo off
cd /d "%~dp0.."
REM Compile 3D shaders for Android

echo Compiling 3D shaders...

REM Check if glslc is available
where glslc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: glslc not found. Make sure Vulkan SDK is installed and in PATH.
    exit /b 1
)

REM Compile vertex shader
glslc plugins\VulkanRenderer\shaders\basic3d.vert -o plugins\VulkanRenderer\shaders\basic3d_vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile basic3d.vert
    exit /b 1
)
echo ✓ Compiled basic3d.vert

REM Compile fragment shader
glslc plugins\VulkanRenderer\shaders\basic3d.frag -o plugins\VulkanRenderer\shaders\basic3d_frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile basic3d.frag
    exit /b 1
)
echo ✓ Compiled basic3d.frag

REM Compile Mega Geometry Vertex Shader
glslc plugins\VulkanRenderer\shaders\mega_geometry.vert -o plugins\VulkanRenderer\shaders\mega_geometry_vert.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile mega_geometry.vert
    exit /b 1
)
echo ✓ Compiled mega_geometry.vert

REM Compile Mega Geometry Fragment Shader
glslc plugins\VulkanRenderer\shaders\mega_geometry.frag -o plugins\VulkanRenderer\shaders\mega_geometry_frag.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile mega_geometry.frag
    exit /b 1
)
echo ✓ Compiled mega_geometry.frag

REM Compile Compute Culling Shader
glslc plugins\VulkanRenderer\shaders\cull.comp -o plugins\VulkanRenderer\shaders\cull_comp.spv
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile cull.comp
    exit /b 1
)
echo ✓ Compiled cull.comp

REM Copy to Android assets
echo Copying shaders to Android assets...
copy /Y plugins\VulkanRenderer\shaders\basic3d_vert.spv android\app\src\main\assets\shaders\basic3d_vert.spv
copy /Y plugins\VulkanRenderer\shaders\basic3d_frag.spv android\app\src\main\assets\shaders\basic3d_frag.spv
copy /Y plugins\VulkanRenderer\shaders\mega_geometry_vert.spv android\app\src\main\assets\shaders\mega_geometry_vert.spv
copy /Y plugins\VulkanRenderer\shaders\mega_geometry_frag.spv android\app\src\main\assets\shaders\mega_geometry_frag.spv
copy /Y plugins\VulkanRenderer\shaders\cull_comp.spv android\app\src\main\assets\shaders\cull_comp.spv

echo ✓ All 3D shaders compiled and copied!
