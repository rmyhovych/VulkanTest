#include "DeviceBuilder.h"
#include "VulkanException.h"

#include <set>
#include <string>

DeviceBuilder::DeviceBuilder(VkInstance instanceHandle, VkSurfaceKHR surfaceHandle) :
	m_instanceHandle(instanceHandle),
	m_surfaceHandle(surfaceHandle)
{
	m_deviceExtentions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

bool DeviceBuilder::isSuitable(VkPhysicalDevice physicalDevice)
{
	bool isPhysicalDeviceSuitable = true;

	uint32_t extentionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount, nullptr);

	std::vector<VkExtensionProperties> extentionProperties(extentionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount, extentionProperties.data());

	std::set<std::string> m_extentionsLeft(m_deviceExtentions.begin(), m_deviceExtentions.end());
	for (VkExtensionProperties& ep : extentionProperties)
	{
		m_extentionsLeft.erase(ep.extensionName);
	}
	isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && m_extentionsLeft.empty();

	if (isPhysicalDeviceSuitable)
	{
		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable &&
			getQueueGraphicsFamilyIndex(physicalDevice, nullptr) &&
			getQueuePresentFamilyIndex(physicalDevice, nullptr, 0);
	}

	if (isPhysicalDeviceSuitable)
	{ 
		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(physicalDevice);

		uint32_t nSurfaceFormats = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceHandle, &nSurfaceFormats, nullptr);

		uint32_t nPresentModes = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceHandle, &nPresentModes, nullptr);

		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && nSurfaceFormats != 0 && nPresentModes != 0;
	}

	return isPhysicalDeviceSuitable;
}

QueueFamilyIndexes DeviceBuilder::getQueueFamilyIndexes(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndexes familyIndexes = { 0, 0 };
	if (!getQueueGraphicsFamilyIndex(physicalDevice, &familyIndexes.graphical))
	{
		throw VulkanException("Device not suitable.");
	}

	if (!getQueuePresentFamilyIndex(physicalDevice, &familyIndexes.present, familyIndexes.graphical))
	{
		throw VulkanException("Device not suitable.");
	}

	return familyIndexes;
}


DeviceConfigurations DeviceBuilder::createDeviceConfigurations(VkPhysicalDevice physicalDevice)
{
	DeviceConfigurations configurations;

	QueueFamilyIndexes queueFamilyIndexes = getQueueFamilyIndexes(physicalDevice);	
	configurations.logicalDevice = createLogicalDevice(physicalDevice, queueFamilyIndexes);
	
	vkGetDeviceQueue(configurations.logicalDevice, queueFamilyIndexes.graphical, 0, &configurations.graphicsQueue);
	vkGetDeviceQueue(configurations.logicalDevice, queueFamilyIndexes.present, 0, &configurations.presentQueue);

	SwapChainSupportDetails swapChainSupportDetails = getSwapChainSupportDetails(physicalDevice);
	configurations.swapchain = createSwapchain(swapChainSupportDetails, configurations.logicalDevice, queueFamilyIndexes);	

	uint32_t nImages = 0;
	vkGetSwapchainImagesKHR(configurations.logicalDevice, configurations.swapchain, &nImages, nullptr);
	configurations.images.resize(nImages);
	vkGetSwapchainImagesKHR(configurations.logicalDevice, configurations.swapchain, &nImages, configurations.images.data());

	return configurations;
}


SwapChainSupportDetails DeviceBuilder::getSwapChainSupportDetails(VkPhysicalDevice physicalDevice)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surfaceHandle, &details.capabilities);

	std::vector<VkSurfaceFormatKHR> swapChainFormats;
	uint32_t nSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceHandle, &nSurfaceFormats, nullptr);
	if (nSurfaceFormats != 0)
	{
		swapChainFormats.resize(nSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceHandle, &nSurfaceFormats, swapChainFormats.data());
	}
	
	std::vector<VkPresentModeKHR> presentModes;
	uint32_t nPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceHandle, &nPresentModes, nullptr);
	if (nPresentModes != 0)
	{
		presentModes.resize(nPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceHandle, &nPresentModes, presentModes.data());
	}

	details.surfaceFormat = chooseSwapSurfaceFormat(swapChainFormats);
	details.presentMode = chooseSwapPresentMode(presentModes);
	details.extent = chooseSwapExtent(details.capabilities);

	return details;
}

VkSurfaceFormatKHR DeviceBuilder::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& swapChainFormats) const
{
	for (const VkSurfaceFormatKHR& format : swapChainFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return swapChainFormats[0];
}

VkPresentModeKHR DeviceBuilder::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
{
	for (const VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	// always avalible
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D DeviceBuilder::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& swapChainCapabilities) const
{
	if (swapChainCapabilities.currentExtent.width != UINT32_MAX)
	{
		return swapChainCapabilities.currentExtent;
	}

	VkExtent2D extentToUse = swapChainCapabilities.minImageExtent;

	// Average extent
	extentToUse.width = (extentToUse.width + swapChainCapabilities.maxImageExtent.width) / 2;
	extentToUse.height = (extentToUse.height + swapChainCapabilities.maxImageExtent.height) / 2;

	return extentToUse;
}

VkDevice DeviceBuilder::createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndexes& familyIndexes)
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(2);
	setQueueCreateInfo(queueCreateInfos[0], familyIndexes.graphical, 1.0f);
	setQueueCreateInfo(queueCreateInfos[1], familyIndexes.present, 1.0f);

	VkDeviceCreateInfo deviceCreateInfo;
	memset(&deviceCreateInfo, 0, sizeof(VkDeviceCreateInfo));

	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();

	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	memset(&physicalDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtentions.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t) m_deviceExtentions.size();

	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;

	VkDevice device;
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create device.");
	}

	return device;
}

VkSwapchainKHR DeviceBuilder::createSwapchain(const SwapChainSupportDetails& swapChainSupportDetails, VkDevice logicalDevice, QueueFamilyIndexes& familyIndexes) const
{
	// minImageCount + 1 to avoid waiting
	uint32_t nImages = swapChainSupportDetails.capabilities.minImageCount + 1;
	
	// VkSurfaceCapabilitiesKHR::maxImageCount == 0 --> No max
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && nImages > swapChainSupportDetails.capabilities.maxImageCount)
	{
		nImages = swapChainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo;
	memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surfaceHandle;
	createInfo.minImageCount = nImages;
	createInfo.imageFormat = swapChainSupportDetails.surfaceFormat.format;
	createInfo.imageColorSpace = swapChainSupportDetails.surfaceFormat.colorSpace;
	createInfo.imageExtent = swapChainSupportDetails.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	if (familyIndexes.graphical != familyIndexes.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = (uint32_t*)&familyIndexes;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapChainSupportDetails.presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain;
	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create swapchain.");
	}

	return swapchain;
}


bool DeviceBuilder::getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(nQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, queueFamilyProperties.data());

	bool found = false;
	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (index != nullptr)
			{ 
				*index = i;
			}
			
			return true;
		}
	}

	return found;
}

bool DeviceBuilder::getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index, uint32_t priorityIndex)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);

	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, priorityIndex, m_surfaceHandle, &supported);
	if (supported)
	{
		if (index != nullptr)
		{
			*index = priorityIndex;
		}

		return true;
	}

	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
		if (i == priorityIndex)
			continue;

		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surfaceHandle, &supported);
		if (supported)
		{
			if (index != nullptr)
			{
				*index = i;
			}

			return true;
		}
	}

	return false;
}

void DeviceBuilder::setQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo, uint32_t index, float priority)
{
	memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));

	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = index;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &priority;
}
	
