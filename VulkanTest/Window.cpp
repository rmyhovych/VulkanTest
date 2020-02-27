#include "Window.h"
#include "VulkanException.h"
#include "PipelineBuilder.h"

#include <iostream>
#include <vector>

Window::Window(int width, int heigth) :
	m_width(width),
	m_height(heigth),

	m_window(nullptr),
	m_surface(VK_NULL_HANDLE),
	m_instance(VK_NULL_HANDLE),

	m_commandPool(VK_NULL_HANDLE),

	m_semaphoreImageAvailable(VK_NULL_HANDLE),
	m_semaphoreRenderFinished(VK_NULL_HANDLE)
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

	VkApplicationInfo appInfo = {};

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Test";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PicoEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;


	VkInstanceCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtentionCount = 0;
	const char** glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

#ifdef DEBUG
	for (uint32_t i = 0; i < glfwExtentionCount; i++)
	{
		printf("%s\n", glfwExtentions[i]);
	}
#endif // !DEBUG

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

	////////////////////
	////// DEVICE //////
	////////////////////

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
#ifdef DEBUG
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
	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
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

	if (vkCreateSemaphore(m_deviceConfigurations.logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoreImageAvailable) != VK_SUCCESS ||
		vkCreateSemaphore(m_deviceConfigurations.logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoreRenderFinished) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create semaphores.");
	}

	
}

void Window::destroy()
{
	if (m_semaphoreImageAvailable != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_deviceConfigurations.logicalDevice, m_semaphoreImageAvailable, nullptr);
		m_semaphoreImageAvailable = VK_NULL_HANDLE;
	}
	if (m_semaphoreRenderFinished != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_deviceConfigurations.logicalDevice, m_semaphoreRenderFinished, nullptr);
		m_semaphoreRenderFinished = VK_NULL_HANDLE;
	}

	if (m_commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_deviceConfigurations.logicalDevice, m_commandPool, nullptr);
		m_commandPool = VK_NULL_HANDLE;
	}

	for (VkFramebuffer framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_deviceConfigurations.logicalDevice, framebuffer, nullptr);
	}
	m_framebuffers.resize(0);

	m_pipelineConfigurations.destroy(m_deviceConfigurations.logicalDevice);
	m_deviceConfigurations.destroy();

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

void Window::draw()
{
	glfwPollEvents();

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_deviceConfigurations.logicalDevice, m_deviceConfigurations.swapchain, UINT64_MAX, 
		m_semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags waitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_semaphoreImageAvailable;
	submitInfo.pWaitDstStageMask = &waitFlag;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphoreRenderFinished;

	if (vkQueueSubmit(m_deviceConfigurations.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw VulkanException("Failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_semaphoreRenderFinished;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_deviceConfigurations.swapchain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_deviceConfigurations.presentQueue, &presentInfo);
}