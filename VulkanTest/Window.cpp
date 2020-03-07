#include "Window.h"
#include "VulkanException.h"
#include "PipelineBuilder.h"

#include <iostream>
#include <vector>

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		printf("\n\tVALIDATION LAYER: %s\n", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

VkResult vkCreateDebugUtilsMessengerEXT_PROXY(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{

	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vkDestroyDebugUtilsMessengerEXT_PROXY(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

#endif // !NDEBUG



Window::Window(int width, int heigth) :
	m_width(width),
	m_height(heigth),

	m_window(nullptr),
	m_surface(VK_NULL_HANDLE),
	m_instance(VK_NULL_HANDLE),

	m_commandPool(VK_NULL_HANDLE),

	m_semaphoresImageAvailable(MAX_FRAMES_IN_FLIGHT),
	m_semaphoresRenderFinished(MAX_FRAMES_IN_FLIGHT),

	m_currentFrameIndex(0)
{
}

Window::~Window()
{
	destroy();
}

void Window::init()
{
	///////////////////////////////
	////// VALIDATION LAYERS //////
	///////////////////////////////

#ifndef NDEBUG
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> avalibleLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avalibleLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool isLayerFound = false;
		for (VkLayerProperties& layerProperty : avalibleLayers)
		{
			if (strcmp(layerName, layerProperty.layerName) == 0) {
				isLayerFound = true;
				break;
			}
		}

		if (isLayerFound != true)
		{
			throw VulkanException("Unable to find layer.");
		}
	}
#endif // !NDEBUG


	////////////////////////////////////////////////////////////////////
	glfwInit();

	//////////////////////
	////// INSTANCE //////
	//////////////////////

#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
	debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerCreateInfo.pfnUserCallback = debugCallback;
	debugMessengerCreateInfo.pUserData = nullptr;
#endif // !NDEBUG

	VkApplicationInfo appInfo = {};

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PicoEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;


	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtentionCount = 0;
	const char** glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

	std::vector<const char*> requiredExtensions(glfwExtentions, glfwExtentions + glfwExtentionCount);

#ifndef NDEBUG
	requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

	instanceCreateInfo.pNext = &debugMessengerCreateInfo;
#else
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;
#endif // !NDEBUG

	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw VulkanException("Failed to initialize instance.");
	}



	/////////////////////////////
	////// DEBUG MESSENGER //////
	/////////////////////////////

#ifndef NDEBUG
	if (vkCreateDebugUtilsMessengerEXT_PROXY(m_instance, &debugMessengerCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create Debug Messenger.");
	}

#endif // !NDEBUG



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




	////////////////////
	////// DEVICE //////
	////////////////////

	DeviceBuilder deviceBuilder(m_surface);

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw VulkanException("No devices supporting vulkan found.");
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkPhysicalDeviceType deviceTypes[] = {
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
		VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
	};

	VkPhysicalDeviceProperties deviceProperties;
	for (VkPhysicalDevice& pd : physicalDevices)
	{
		vkGetPhysicalDeviceProperties(pd, &deviceProperties);
		if (deviceProperties.deviceType == deviceTypes[1])
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

	m_deviceConfigurations = deviceBuilder.createDeviceConfigurations(physicalDevice);



	//////////////////////
	////// PIPELINE //////
	//////////////////////

	PipelineBuilder shaderBuilder(m_deviceConfigurations);
	m_pipelineConfigurations = shaderBuilder.createGraphicsPipeline("shaders/bin/triangle.vert.spv", "shaders/bin/triangle.frag.spv");



	//////////////////////////
	////// FRAMEBUFFERS //////
	//////////////////////////

	m_framebuffers.resize(m_deviceConfigurations.imageViews.size());
#ifndef NDEBUG
	printf("Creating [%d] framebuffers.", m_framebuffers.size());
#endif // DEBUG

	for (int i = m_deviceConfigurations.imageViews.size() - 1; i >= 0; --i)
	{
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_pipelineConfigurations.renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &m_deviceConfigurations.imageViews[i];
		framebufferCreateInfo.width = m_deviceConfigurations.swapchainSupportDetails.extent.width;
		framebufferCreateInfo.height = m_deviceConfigurations.swapchainSupportDetails.extent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(m_deviceConfigurations.logicalDevice, &framebufferCreateInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create framebuffer.");
		}
	}



	///////////////////////////
	////// COMMAND POOLS //////
	///////////////////////////

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = m_deviceConfigurations.queueFamilyIndexes.graphical;
	commandPoolCreateInfo.flags = 0;

	if (vkCreateCommandPool(m_deviceConfigurations.logicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create command pool.");
	}



	/////////////////////////////
	////// COMMAND BUFFERS //////
	/////////////////////////////

	m_commandBuffers.resize(m_framebuffers.size());
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = m_commandBuffers.size();

	if (vkAllocateCommandBuffers(m_deviceConfigurations.logicalDevice, &commandBufferAllocateInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw VulkanException("Failed to allocate command buffers.");
	}



	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_pipelineConfigurations.renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_deviceConfigurations.swapchainSupportDetails.extent;
	VkClearValue clearColor = { 0.0f, 0.5f, 0.5f, 0.0f };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;


	for (int i = m_commandBuffers.size() - 1; i >= 0; --i)
	{
		if (vkBeginCommandBuffer(m_commandBuffers[i], &commandBufferBeginInfo) != VK_SUCCESS)
		{
			throw VulkanException("Failed to begin command buffer.");
		}

		renderPassBeginInfo.framebuffer = m_framebuffers[i];

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineConfigurations.graphicsPipeline);
		vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(m_commandBuffers[i]);

		if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to end command buffer.");
		}
	}



	////////////////////////
	////// SEMAPHORES //////
	////////////////////////

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_deviceConfigurations.logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoresImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_deviceConfigurations.logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoresRenderFinished[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create semaphores.");
		}
	}

}

void Window::destroy()
{
#ifndef NDEBUG
	if (m_debugMessenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT_PROXY(m_instance, m_debugMessenger, nullptr);
	}
#endif // !NDEBUG

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_deviceConfigurations.logicalDevice, m_semaphoresImageAvailable[i], nullptr);
		vkDestroySemaphore(m_deviceConfigurations.logicalDevice, m_semaphoresRenderFinished[i], nullptr);
	}

	vkDestroyCommandPool(m_deviceConfigurations.logicalDevice, m_commandPool, nullptr);

	for (VkFramebuffer framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_deviceConfigurations.logicalDevice, framebuffer, nullptr);
	}

	m_pipelineConfigurations.destroy(m_deviceConfigurations.logicalDevice);
	m_deviceConfigurations.destroy();

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);

	glfwTerminate();
}

bool Window::isOpen()
{
	return glfwWindowShouldClose(m_window) == 0;
}

void Window::draw()
{
	glfwPollEvents();

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_deviceConfigurations.logicalDevice, m_deviceConfigurations.swapchain, UINT64_MAX,
		m_semaphoresImageAvailable[m_currentFrameIndex], VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags waitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_semaphoresImageAvailable[m_currentFrameIndex];
	submitInfo.pWaitDstStageMask = &waitFlag;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphoresRenderFinished[m_currentFrameIndex];

	if (vkQueueSubmit(m_deviceConfigurations.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw VulkanException("Failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_semaphoresRenderFinished[m_currentFrameIndex];

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_deviceConfigurations.swapchain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_deviceConfigurations.presentQueue, &presentInfo);
	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkDevice Window::getDevice()
{
	return m_deviceConfigurations.logicalDevice;
}
