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
			getQueuePresentFamilyIndex(physicalDevice, nullptr);
	}

	if (isPhysicalDeviceSuitable)
	{ 
		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(physicalDevice);
		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable &&
			!swapChainDetails.formats.empty() &&
			!swapChainDetails.presentModes.empty();
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

	if (!getQueuePresentFamilyIndex(physicalDevice, &familyIndexes.present))
	{
		throw VulkanException("Device not suitable.");
	}

	return familyIndexes;
}

SwapChainSupportDetails DeviceBuilder::getSwapChainSupportDetails(VkPhysicalDevice physicalDevice)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surfaceHandle, &details.capabilities);

	uint32_t nSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceHandle, &nSurfaceFormats, nullptr);
	if (nSurfaceFormats != 0)
	{
		details.formats.resize(nSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceHandle, &nSurfaceFormats, details.formats.data());
	}
	
	uint32_t nPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceHandle, &nPresentModes, nullptr);
	if (nPresentModes != 0)
	{
		details.presentModes.resize(nPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceHandle, &nPresentModes, details.presentModes.data());
	}

	return details;
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

bool DeviceBuilder::getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);

	VkBool32 supported;
	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
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
	
