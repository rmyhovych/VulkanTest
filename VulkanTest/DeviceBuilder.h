#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "DeviceConfigurations.h"


class DeviceBuilder
{
public:
	DeviceBuilder(VkInstance instanceHandle, VkSurfaceKHR surfaceHandle);

	bool isSuitable(VkPhysicalDevice physicalDevice);

	DeviceConfigurations createDeviceConfigurations(VkPhysicalDevice physicalDevice);

private:
	VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndexes& familyIndexes);

	
	// QUEUES
	QueueFamilyIndexes getQueueFamilyIndexes(VkPhysicalDevice physicalDevice);
	
	bool getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index);
	bool getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index, uint32_t priorityIndex);

	void setQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo, uint32_t index, float priority);


	// SWAPCHAIN
	VkSwapchainKHR createSwapchain(const SwapChainSupportDetails& swapChainSupportDetails, VkDevice logicalDevice, QueueFamilyIndexes& familyIndexes) const;

	SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice physicalDevice);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& swapChainFormats) const;
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& swapChainCapabilities) const;

	void createImageViews(DeviceConfigurations& configurations) const;

private:
	VkInstance m_instanceHandle;
	VkSurfaceKHR m_surfaceHandle;

	std::vector<const char*> m_deviceExtentions;
};
