#include "Window.h"
#include "VulkanException.h"
#include "DeviceBuilder.h"

#include <stdexcept>
#include <iostream>

Window::Window(int width, int heigth) :
	m_width(width),
	m_height(heigth),

	m_window(NULL),
	m_instance(VK_NULL_HANDLE)
{
}

Window::~Window()
{
	destroy();
}

void Window::init()
{
	glfwInit();

	//////////////////////
	////// INSTANCE //////
	//////////////////////

	VkApplicationInfo appInfo;
	memset(&appInfo, 0, sizeof(VkApplicationInfo));

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PicoEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;


	VkInstanceCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtentionCount = 0;
	const char** glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

	createInfo.ppEnabledExtensionNames = glfwExtentions;
	createInfo.enabledExtensionCount = glfwExtentionCount;

	createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, NULL, &m_instance) != VK_SUCCESS)
	{
		throw VulkanException("Failed to initialize instance.");
	}



	////////////////////
	////// WINDOW //////
	////////////////////

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(m_width, m_height, "Vulkan", NULL, NULL);

	if (m_window == NULL)
	{
		glfwTerminate();
		throw VulkanException("Failed to create GLFW window.");
	}


	if (glfwCreateWindowSurface(m_instance, m_window, NULL, &m_surface) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create window surface.");
	}

	/////////////////////////////
	////// PHYSICAL DEVICE //////
	/////////////////////////////

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, NULL);

	if (deviceCount == 0)
	{
		throw VulkanException("No devices supporting vulkan found.");
	}

	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[deviceCount];
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices);

	uint32_t graphicsQueueFamily = 0;
	uint32_t presentQueueFamily = 0;

	for (int i = 0; i < deviceCount; ++i)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			DeviceBuilder deviceBuilder(m_instance, physicalDevices[i]);

			if (deviceBuilder.isSuitable(m_surface))
			{
				physicalDevice = physicalDevices[i];
				break;
			}
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		physicalDevice = physicalDevices[0];

		if (!getQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT, &graphicsQueueFamily))
		{
			delete[] physicalDevices;
			physicalDevices = NULL;

			throw VulkanException("No Physical Device supports graphics.");
		}

	}

	delete[] physicalDevices;
	physicalDevices = NULL;



	////////////////////////////
	////// LOGICAL DEVICE //////
	////////////////////////////

	VkDeviceQueueCreateInfo deviceQueueCreateInfo;
	memset(&deviceQueueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));

	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	deviceQueueCreateInfo.queueCount = 1;
	float queuePriority = 1.0f;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;


	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	memset(&physicalDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));


	VkDeviceCreateInfo deviceCreateInfo;
	memset(&deviceCreateInfo, 0, sizeof(VkDeviceCreateInfo));

	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = 0;
	deviceCreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, NULL, &m_device) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create device.");
	}



	//////////////////////////
	////// DEVICE QUEUE //////
	//////////////////////////

	vkGetDeviceQueue(m_device, queueFamilyIndex, 0, &m_graphicsQueue);
}

void Window::destroy()
{
	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, NULL);
		m_device = VK_NULL_HANDLE;
	}

	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, NULL);
		m_surface = VK_NULL_HANDLE;
	}

	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, NULL);
		m_instance = VK_NULL_HANDLE;
	}

	if (m_window != NULL)
	{
		glfwDestroyWindow(m_window);
		m_window = NULL;

		glfwTerminate();
	}
}

bool Window::isOpen()
{
	return glfwWindowShouldClose(m_window) == 0;
}

void Window::swapBuffers()
{
	glfwSwapBuffers(m_window);
	glfwPollEvents();
}
