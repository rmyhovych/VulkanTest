#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class DeviceBuilder
{
public:
	DeviceBuilder(VkInstance instanceHandle, VkSurfaceKHR surfaceHandle, VkPhysicalDevice physicalDevice);

	bool isSuitable();
	
	void init();
	void destroy();

private:
	bool getQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits queueFlagBit, uint32_t* index);
	bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice);

private:
	VkInstance m_instanceHandle;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
};
