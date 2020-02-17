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
	uint32_t extentionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extentionCount, NULL);

	std::vector<VkExtensionProperties> extentionProperties(extentionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extentionCount, extentionProperties.data());

	std::set<std::string> m_extentionsLeft(m_deviceExtentions.begin(), m_deviceExtentions.end());
	for (VkExtensionProperties& ep : extentionProperties)
	{
		m_extentionsLeft.erase(ep.extensionName);
	}

	return m_extentionsLeft.empty() && 
		getQueueGraphicsFamilyIndex(physicalDevice, NULL) && 
		getQueuePresentFamilyIndex(physicalDevice, NULL);
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
	deviceCreateInfo.ppEnabledLayerNames = NULL;

	VkDevice device;
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create device.");
	}

	return device;
}


bool DeviceBuilder::getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, NULL);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(nQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, queueFamilyProperties.data());

	bool found = false;
	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (index != NULL)
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
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, NULL);

	VkBool32 supported;
	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surfaceHandle, &supported);
		if (supported)
		{
			if (index != NULL)
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
	
