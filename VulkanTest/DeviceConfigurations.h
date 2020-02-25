#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>


struct QueueFamilyIndexes
{
	uint32_t graphical;
	uint32_t present;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

struct DeviceConfigurations
{
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	SwapChainSupportDetails swapchainSupportDetails;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> images;

	void destroy()
	{
		vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
		vkDestroyDevice(logicalDevice, nullptr);
	}
};
