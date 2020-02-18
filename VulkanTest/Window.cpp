#include "Window.h"
#include "VulkanException.h"
#include "DeviceBuilder.h"

#include <iostream>
#include <vector>

Window::Window(int width, int heigth) :
	m_width(width),
	m_height(heigth),

	m_window(nullptr),
	m_surface(VK_NULL_HANDLE),

	m_instance(VK_NULL_HANDLE),
	m_device(VK_NULL_HANDLE),

	m_graphicsQueue(VK_NULL_HANDLE),
	m_presentQueue(VK_NULL_HANDLE)
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

#ifndef NDEBUG
	for (uint32_t i = 0; i < glfwExtentionCount; i++)
	{
		printf("%s\n", glfwExtentions[i]);
	}
#endif // !NDEBUG

	createInfo.ppEnabledExtensionNames = glfwExtentions;
	createInfo.enabledExtensionCount = glfwExtentionCount;

	createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw VulkanException("Failed to initialize instance.");
	}



	////////////////////
	////// WINDOW //////
	////////////////////

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	 
	m_window = glfwCreateWindow(m_width, m_height, "Vulkan", nullptr, nullptr);

	if (m_window == nullptr)
	{
		glfwTerminate();
		throw VulkanException("Failed to create GLFW window.");
	}


	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create window surface.");
	}

	/////////////////////////////
	////// PHYSICAL DEVICE //////
	/////////////////////////////

	DeviceBuilder deviceBuilder(m_instance, m_surface);

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw VulkanException("No devices supporting vulkan found.");
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());
	
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties deviceProperties;
	for (VkPhysicalDevice& pd : physicalDevices)
	{
		vkGetPhysicalDeviceProperties(pd, &deviceProperties);
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			if (deviceBuilder.isSuitable(pd))
			{
				physicalDevice = pd;
 				break;
			}
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		physicalDevice = physicalDevices[0];
		
		if (!deviceBuilder.isSuitable(physicalDevice))
		{
			throw VulkanException("No device is suitable.");
		}
	}


	////////////////////////////
	////// LOGICAL DEVICE //////
	////////////////////////////

	QueueFamilyIndexes familyIndexes = deviceBuilder.getQueueFamilyIndexes(physicalDevice);
	m_device = deviceBuilder.createLogicalDevice(physicalDevice, familyIndexes);

	//////////////////////////
	////// DEVICE QUEUE //////
	//////////////////////////

	VkQueue graphicsQueue;
	vkGetDeviceQueue(m_device, familyIndexes.graphical, 0, &graphicsQueue);

	VkQueue presentQueue;
	vkGetDeviceQueue(m_device, familyIndexes.present, 0, &presentQueue);
}

void Window::destroy()
{
	if (m_device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}

	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}

	if (m_window != nullptr)
	{
		glfwDestroyWindow(m_window);
		m_window = nullptr;

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
