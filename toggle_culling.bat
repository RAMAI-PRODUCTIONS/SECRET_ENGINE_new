@echo off
echo ========================================
echo Toggle Camera Culling
echo ========================================
echo.
echo Current Status: CULLING DISABLED
echo.
echo To re-enable culling:
echo 1. Edit plugins\VulkanRenderer\shaders\cull.comp
echo 2. Change "return true;" to use original culling code
echo 3. Run: tools\compile_3d_shaders.bat
echo 4. Run: deploy_android.bat
echo.
echo To verify culling status:
echo - Look at triangle count in debug overlay
echo - With culling OFF: count stays constant
echo - With culling ON: count changes when you rotate camera
echo.
pause
