# CMakeList.txt : CMake project for VulkanTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.

set (
	VulkanTest_HDRS

	Window.h
	VulkanException.h
	FileReader.h
	camera/Camera.h
	camera/FocusedCamera.h
)

set (
	VulkanTest_SRC

	main.cpp

	Window.cpp
	FileReader.cpp
	camera/Camera.cpp
	camera/FocusedCamera.cpp
)

add_executable (
	VulkanTest 
	
	${VulkanTest_SRC} 
	${VulkanTest_HDRS} )

target_include_directories (
	VulkanTest
	
	PUBLIC

	${LIBS_PATH}/glfw-3.3.2/include
	${LIBS_PATH}/glm-0.9.9.6
	${VULKAN_PATH}/Include
)

target_link_libraries (
	VulkanTest 

	${LIBS_PATH}/glfw-3.3.2/lib/glfw3.lib
	${VULKAN_PATH}/Lib/vulkan-1.lib
)

add_custom_target(
	ShaderCompilation 
	COMMAND cmd /c ${PROJECT_SOURCE_DIR}/shader_compile.bat
)

add_dependencies(VulkanTest ShaderCompilation)