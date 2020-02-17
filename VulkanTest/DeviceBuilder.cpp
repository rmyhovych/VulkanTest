#include "DeviceBuilder.h"
#include "VulkanException.h"

DeviceBuilder::DeviceBuilder(VkInstance instanceHandle, VkSurfaceKHR surfaceHandle, VkPhysicalDevice physicalDevice) :
	m_instanceHandle(instanceHandle),
	m_physicalDevice(physicalDevice)
{
}

bool DeviceBuilder::isSuitable()
{

	if (!getQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT, &graphicsQueueFamily))
	{
		throw VulkanException("No Physical Device supports graphics.");
	}

}

void DeviceBuilder::destroy()
{
}
