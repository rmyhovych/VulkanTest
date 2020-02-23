set SHADERS_PATH=%~dp0/shaders

set SRC_PATH=%SHADERS_PATH%/src
set BIN_PATH=%SHADERS_PATH%/bin

set FILES=triangle.vert;triangle.frag

set GLSLC_EXE=C:/VulkanSDK/1.2.131.2/Bin/glslc.exe

for %%a in (%FILES%) do (
	%GLSLC_EXE% %SRC_PATH%/%%a -o %BIN_PATH%/%%a.spv
)
