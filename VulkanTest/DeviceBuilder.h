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
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class DeviceBuilder
{
public:
	DeviceBuilder(VkInstance instanceHandle, VkSurfaceKHR surfaceHandle);

	bool isSuitable(VkPhysicalDevice physicalDevice);

	QueueFamilyIndexes getQueueFamilyIndexes(VkPhysicalDevice physicalDevice);
	VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndexes& familyIndexes);
	VkSwapchainKHR createSwapchain(VkPhysicalDevice physicalDevice);

private:
	bool getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index);
	bool getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index);

	void setQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo, uint32_t index, float priority);


	SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice physicalDevice);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(SwapChainSupportDetails& swapChainDetails);
	VkPresentModeKHR chooseSwapPresentMode(SwapChainSupportDetails& swapChainDetails);
	VkExtent2D chooseSwapExtent(SwapChainSupportDetails& swapChainDetails);

private:
	VkInstance m_instanceHandle;
	VkSurfaceKHR m_surfaceHandle;

	std::vector<const char*> m_deviceExtentions;
};
