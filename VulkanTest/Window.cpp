#include "Window.h"
#include "VulkanException.h"
#include "PipelineBuilder.h"

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>

const uint32_t MAX_FRAMES_IN_FLIGHT = 5;

const std::vector<const char*> DEVICE_EXTENSIONS({
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	});


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

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

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
	m_inFlightFences(MAX_FRAMES_IN_FLIGHT),

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

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> avalibleLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avalibleLayers.data());

	for (const char* layerName : VALIDATION_LAYERS)
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

	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

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
			if (isDeviceSuitable(pd, m_surface))
			{
				physicalDevice = pd;
				break;
			}
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		physicalDevice = physicalDevices[0];

		if (!isDeviceSuitable(physicalDevice, m_surface))
		{
			throw VulkanException("No device is suitable.");
		}
	}

	// Device
	m_queueFamilyIndexes = getQueueFamilyIndexes(physicalDevice);
	m_logicalDevice = createLogicalDevice(physicalDevice, m_queueFamilyIndexes);

	// Queues
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndexes.graphical, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndexes.present, 0, &m_presentQueue);

	// Swapchain
	m_swapchainSupportDetails = getSwapChainSupportDetails(physicalDevice, m_surface, m_width, m_height);
	m_swapchain = createSwapchain(m_swapchainSupportDetails, m_logicalDevice, m_surface, m_queueFamilyIndexes);

	// Images
	uint32_t nImages = 0;
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &nImages, nullptr);
	m_images.resize(nImages);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &nImages, m_images.data());

	// Image Views
	m_imageViews = createImageViews(m_logicalDevice, m_images, m_swapchainSupportDetails);



	//////////////////////
	////// PIPELINE //////
	//////////////////////

	PipelineBuilder shaderBuilder(m_logicalDevice, m_swapchainSupportDetails);
	m_pipelineConfigurations = shaderBuilder.createGraphicsPipeline("shaders/bin/triangle.vert.spv", "shaders/bin/triangle.frag.spv");



	//////////////////////////
	////// FRAMEBUFFERS //////
	//////////////////////////

	m_framebuffers.resize(m_imageViews.size());
#ifndef NDEBUG
	printf("Creating [%d] framebuffers.", m_framebuffers.size());
#endif // DEBUG

	for (int i = m_imageViews.size() - 1; i >= 0; --i)
	{
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_pipelineConfigurations.renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &m_imageViews[i];
		framebufferCreateInfo.width = m_swapchainSupportDetails.extent.width;
		framebufferCreateInfo.height = m_swapchainSupportDetails.extent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(m_logicalDevice, &framebufferCreateInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create framebuffer.");
		}
	}



	///////////////////////////
	////// COMMAND POOLS //////
	///////////////////////////

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = m_queueFamilyIndexes.graphical;
	commandPoolCreateInfo.flags = 0;

	if (vkCreateCommandPool(m_logicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
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

	if (vkAllocateCommandBuffers(m_logicalDevice, &commandBufferAllocateInfo, m_commandBuffers.data()) != VK_SUCCESS)
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
	renderPassBeginInfo.renderArea.extent = m_swapchainSupportDetails.extent;
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
		if (vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoresImageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoresRenderFinished[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create semaphores.");
		}
	}



	////////////////////
	////// FENCES //////
	////////////////////

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateFence(m_logicalDevice, &fenceCreateInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create fences.");
		}
	}


	//// IMAGE FENCES ////
	m_imagesInFlight.resize(m_images.size(), VK_NULL_HANDLE);

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
		vkDestroySemaphore(m_logicalDevice, m_semaphoresImageAvailable[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_semaphoresRenderFinished[i], nullptr);
		vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);

	for (VkFramebuffer framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
	}

	m_pipelineConfigurations.destroy(m_logicalDevice);
	for (VkImageView imageView : m_imageViews)
	{
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
	vkDestroyDevice(m_logicalDevice, nullptr);

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
	vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX,
		m_semaphoresImageAvailable[m_currentFrameIndex], VK_NULL_HANDLE, &imageIndex);

	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		printf("DONE %d\n", imageIndex);
		vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrameIndex];

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

	vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrameIndex]);
	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrameIndex]) != VK_SUCCESS)
	{
		throw VulkanException("Failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_semaphoresRenderFinished[m_currentFrameIndex];

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkDevice Window::getDevice()
{
	return m_logicalDevice;
}


bool Window::isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle)
{
	bool isPhysicalDeviceSuitable = true;

	uint32_t extentionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount, nullptr);

	std::vector<VkExtensionProperties> extentionProperties(extentionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount, extentionProperties.data());

	std::set<std::string> m_extentionsLeft(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
	for (VkExtensionProperties& ep : extentionProperties)
	{
		m_extentionsLeft.erase(ep.extensionName);
	}
	isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && m_extentionsLeft.empty();

	if (isPhysicalDeviceSuitable)
	{
		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable &&
			getQueueGraphicsFamilyIndex(physicalDevice, nullptr) &&
			getQueuePresentFamilyIndex(physicalDevice, m_surface, nullptr, 0);
	}

	if (isPhysicalDeviceSuitable)
	{
		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(physicalDevice, m_surface, m_width, m_height);

		uint32_t nSurfaceFormats = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &nSurfaceFormats, nullptr);

		uint32_t nPresentModes = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &nPresentModes, nullptr);

		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && nSurfaceFormats != 0 && nPresentModes != 0;
	}

	return isPhysicalDeviceSuitable;
}

QueueFamilyIndexes Window::getQueueFamilyIndexes(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndexes familyIndexes = { 0, 0 };
	if (!getQueueGraphicsFamilyIndex(physicalDevice, &familyIndexes.graphical))
	{
		throw VulkanException("Device not suitable.");
	}

	if (!getQueuePresentFamilyIndex(physicalDevice, m_surface, &familyIndexes.present, familyIndexes.graphical))
	{
		throw VulkanException("Device not suitable.");
	}

	return familyIndexes;
}

SwapChainSupportDetails Window::getSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t width, uint32_t height)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surfaceHandle, &details.capabilities);

	std::vector<VkSurfaceFormatKHR> swapChainFormats;
	uint32_t nSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &nSurfaceFormats, nullptr);
	if (nSurfaceFormats != 0)
	{
		swapChainFormats.resize(nSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &nSurfaceFormats, swapChainFormats.data());
	}

	std::vector<VkPresentModeKHR> presentModes;
	uint32_t nPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &nPresentModes, nullptr);
	if (nPresentModes != 0)
	{
		presentModes.resize(nPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &nPresentModes, presentModes.data());
	}

	details.surfaceFormat = chooseSwapSurfaceFormat(swapChainFormats);
	details.presentMode = chooseSwapPresentMode(presentModes);
	details.extent = chooseSwapExtent(details.capabilities, width, height);

	return details;
}


VkSurfaceFormatKHR Window::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& swapChainFormats)
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

VkPresentModeKHR Window::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
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

VkExtent2D Window::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& swapChainCapabilities, uint32_t width, uint32_t height)
{
	if (swapChainCapabilities.currentExtent.width != UINT32_MAX)
	{
		return swapChainCapabilities.currentExtent;
	}

	VkExtent2D wantedExtent = { width, height };
	VkExtent2D extentToUse = {};

	// Average extent
	extentToUse.width = std::max(swapChainCapabilities.maxImageExtent.width,
		std::min(swapChainCapabilities.maxImageExtent.width, wantedExtent.width));
	extentToUse.height = std::max(swapChainCapabilities.maxImageExtent.height,
		std::min(swapChainCapabilities.maxImageExtent.height, wantedExtent.height));

	return { 5, 20 };
}

std::vector<VkImageView> Window::createImageViews(VkDevice logicalDevice, std::vector<VkImage>& images, SwapChainSupportDetails& swapchainSupportDetails) const
{
	std::vector<VkImageView> imageViews(images.size());

	for (int i = imageViews.size() - 1; i >= 0; --i)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = images[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainSupportDetails.surfaceFormat.format;

		createInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create image view.");
		}
	}

	return imageViews;
}

VkDevice Window::createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndexes& familyIndexes)
{
	std::set<uint32_t> uniqueQueueFamilies = { familyIndexes.graphical, familyIndexes.present };
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
	{
		queueCreateInfos.push_back(VkDeviceQueueCreateInfo());
		setQueueCreateInfo(queueCreateInfos[queueCreateInfos.size() - 1], queueFamilyIndex, 1.0f);
	}


	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t) DEVICE_EXTENSIONS.size();

#ifndef NDEBUG
	deviceCreateInfo.enabledLayerCount = (uint32_t)VALIDATION_LAYERS.size();
	deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#else
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif // !NDEBUG


	VkDevice device;
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create device.");
	}

	return device;
}

VkSwapchainKHR Window::createSwapchain(const SwapChainSupportDetails& swapChainSupportDetails, VkDevice logicalDevice, VkSurfaceKHR surfaceHandle, QueueFamilyIndexes& familyIndexes) const
{
	// minImageCount + 1 to avoid waiting
	uint32_t nImages = swapChainSupportDetails.capabilities.minImageCount + 1;

	// VkSurfaceCapabilitiesKHR::maxImageCount == 0 --> No max
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && nImages > swapChainSupportDetails.capabilities.maxImageCount)
	{
		nImages = swapChainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surfaceHandle;
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

bool Window::getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index)
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

bool Window::getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t* index, uint32_t priorityIndex)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);

	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, priorityIndex, surfaceHandle, &supported);
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

		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surfaceHandle, &supported);
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

void Window::setQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo, uint32_t index, float priority)
{

	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = index;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &priority;
}