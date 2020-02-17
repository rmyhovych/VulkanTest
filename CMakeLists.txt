# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("VulkanTest")

set (VULKAN_PATH C:/VulkanSDK/1.2.131.2)
get_filename_component (LIBS_PATH libs REALPATH)

# Include sub-projects.
add_subdirectory (VulkanTest)
