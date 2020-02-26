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

	QueueFamilyIndexes queueFamilyIndexes;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	SwapChainSupportDetails swapchainSupportDetails;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;

	void destroy()
	{
		for (VkImageView imageView : imageViews)
		{
			vkDestroyImageView(logicalDevice, imageView, nullptr);
		}
		imageViews.resize(0);

		if (swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
			swapchain = VK_NULL_HANDLE;
		}
		if (logicalDevice != VK_NULL_HANDLE)
		{
			vkDestroyDevice(logicalDevice, nullptr);
			logicalDevice = VK_NULL_HANDLE;
		}
	}
};
