#include "Window.h"
#include "VulkanException.h"
#include "FileReader.h"

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>


#include <ctime>

#define CALL_VK(result)														\
    if (VK_SUCCESS != (result)) {											\
        printf("Vulkan error. File[%s], line[%d]", __FILE__, __LINE__);		\
        assert(false);														\
        }


class Profiler
{
public:
	Profiler(uint16_t nIterations = 1000) :
		nIterations_(nIterations),
		currentIteration_(0),
		timePassed_(0)
	{
		std::streambuf* orig_buff = std::cout.rdbuf();
		std::cout.rdbuf(NULL);

		uint32_t iterToTest = 1000;
		std::clock_t startTime = std::clock();
		for (uint32_t i = 0; i < iterToTest; ++i)
		{
			for (uint32_t j = 0; j < nIterations_; ++j)
			{
				start();
				end();
			}
		}
		std::clock_t timePassed = std::clock() - startTime;
		timeToIgnore_ = (double)(timePassed / iterToTest);

		std::cout.rdbuf(orig_buff);

		startClock_ = clock();
		timePassed_ = 0;
		currentIteration_ = 0;
	}

	void start()
	{
		startClock_ = std::clock();
	}

	void end()
	{
		timePassed_ += (double)(std::clock() - startClock_);
		++currentIteration_;

		if (currentIteration_ == nIterations_)
		{
			std::cout << "\nFPS: " << 1000.0 / (((timePassed_ - timeToIgnore_) / nIterations_));
			currentIteration_ = 0;
			timePassed_ = 0;
		}
	}

private:

	const uint16_t nIterations_;
	uint16_t currentIteration_;

	double timeToIgnore_;

	std::clock_t startClock_;
	double timePassed_;
};


const uint32_t MAX_FRAMES_IN_FLIGHT = 5;

const std::vector<const char*> DEVICE_EXTENSIONS({
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	});


#ifndef NDEBUG
/*
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
*/

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	const char validation[] = "VALIDATION";
	const char performance[] = "Performance";
	const char error[] = "ERROR";
	const char warning[] = "WARNING";
	const char unknownType[] = "UNKNOWN_TYPE";
	const char unknownSeverity[] = "UNKNOWN_SEVERITY";
	const char* typeString = unknownType;
	const char* severityString = unknownSeverity;
	const char* messageIdName = callbackData->pMessageIdName;
	int32_t messageIdNumber = callbackData->messageIdNumber;
	const char* message = callbackData->pMessage;

	if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		return VK_FALSE;
	}

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		severityString = error;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		severityString = warning;
	}
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		typeString = validation;
	}
	else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		typeString = performance;
	}

	printf("\n%s %s:\n\t[%s] Code %i : %s\n\n",
		typeString,
		severityString,
		messageIdName,
		messageIdNumber,
		message);

	// Returning false tells the layer not to stop when the event occurs, so
	// they see the same behavior with and without validation layers enabled.
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


void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	Window* windowObject = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	windowObject->setResized();
}



const VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;


Window::Window(int width, int heigth) :
	m_xpos(0),
	m_ypos(0),
	m_isPressed(false),

	m_width(width),
	m_height(heigth),
	m_camera(width, heigth),

	m_window(nullptr),
	m_surface(VK_NULL_HANDLE),
	m_instance(VK_NULL_HANDLE),

	m_commandPool(VK_NULL_HANDLE),
	m_swapchain(VK_NULL_HANDLE),

	m_semaphoresImageAvailable(MAX_FRAMES_IN_FLIGHT),
	m_semaphoresRenderFinished(MAX_FRAMES_IN_FLIGHT),
	m_inFlightFences(MAX_FRAMES_IN_FLIGHT),

	m_currentFrameIndex(0),

	m_vertices({
		{{-0.5f, -0.5f, 0.0f }, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f }, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f }, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		}),

	m_indexes({ 0, 1, 2, 0, 2, 3 })
{
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

	glfwInit();


	createInstance();
	createWindow();
	createDevice();
	createSwapChain();
	m_imageViews = createImageViews(m_logicalDevice, m_images, m_swapchainSupportDetails);
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline("shaders/bin/triangle.vert.spv", "shaders/bin/triangle.frag.spv");

	createDepthResources();

	createFramebuffers();
	createCommandPool();

	createTextureImage();
	createTextureImageView();
	createTextureSampler();

	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	createCommandBuffers();
	createSyncObjects();
}

void Window::destroy()
{
	cleanupSwapChain();

	vkDestroySampler(m_logicalDevice, m_textureSampler, nullptr);
	vkDestroyImageView(m_logicalDevice, m_textureImageView, nullptr);

	vkDestroyImage(m_logicalDevice, m_textureImage, nullptr);
	vkFreeMemory(m_logicalDevice, m_textureImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(m_logicalDevice, m_uboDescriptorSetLayout, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_logicalDevice, m_semaphoresImageAvailable[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_semaphoresRenderFinished[i], nullptr);
		vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
	vkDestroyDevice(m_logicalDevice, nullptr);

#ifndef NDEBUG
	vkDestroyDebugUtilsMessengerEXT_PROXY(m_instance, m_debugMessenger, nullptr);
#endif // !NDEBUG

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);

	glfwTerminate();
}

bool Window::isOpen()
{
	return glfwWindowShouldClose(m_window) == 0;
}

static glm::mat4 mooodel = glm::mat4(1);



Profiler profiler(500);

#include <thread>

void Window::draw()
{
	profiler.start();
	double xpos, ypos;
	glfwGetCursorPos(m_window, &xpos, &ypos);

	if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		printf("sleep\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	bool isPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	if (isPressed && !m_isPressed)
	{
		m_xpos = xpos;
		m_ypos = ypos;
	}

	m_isPressed = isPressed;

	if (isPressed)
	{
		float force = 0.005;
		m_camera.rotate(force * (xpos - m_xpos), force * (ypos - m_ypos));
	}

	m_xpos = xpos;
	m_ypos = ypos;

	glfwPollEvents();
	vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult imageResult = vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX,
		m_semaphoresImageAvailable[m_currentFrameIndex], VK_NULL_HANDLE, &imageIndex);

	if (imageResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR)
	{
		throw VulkanException("Failed to acquire next image.");
	}


	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrameIndex];

	/////////////////////////////////
	UniformBufferObject ubo = {};
	mooodel = glm::rotate_slow(mooodel, glm::pi<float>() / 1800, glm::vec3(0, 0, 1));
	ubo.model = mooodel;
	ubo.view = m_camera.getView();
	ubo.projection = m_camera.getProjection();
	ubo.projection[1][1] *= -1;

	void* data;
	vkMapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2)], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2)]);
	/////////////////////////////////
	/////////////////////////////////
	ubo.model = glm::mat4(1);
	ubo.model = glm::translate(ubo.model, { 1,2,-1 });

	vkMapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2) + 1], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2) + 1]);
	/////////////////////////////////
	
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

	imageResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (imageResult == VK_ERROR_OUT_OF_DATE_KHR || imageResult == VK_SUBOPTIMAL_KHR || m_framebufferResized)
	{
		m_framebufferResized = false;
		recreateSwapChain();
	}
	else if (imageResult != VK_SUCCESS)
	{
		throw VulkanException("Failder to present image.");
	}

	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	profiler.end();
}

VkDevice Window::getDevice()
{
	return m_logicalDevice;
}

void Window::setResized()
{
	m_framebufferResized = true;
}

void Window::createInstance()
{
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

}

void Window::createWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(m_width, m_height, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

	if (m_window == nullptr)
	{
		glfwTerminate();
		throw VulkanException("Failed to create GLFW window.");
	}

	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create window surface.");
	}
}

void Window::createDevice()
{
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
	m_physicalDevice = physicalDevice;


	// Device
	m_queueFamilyIndexes = getQueueFamilyIndexes(physicalDevice);
	m_logicalDevice = createLogicalDevice(physicalDevice, m_queueFamilyIndexes);

	// Queues
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndexes.graphical, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndexes.present, 0, &m_presentQueue);
}

void Window::createSwapChain()
{
	// Swapchain
	glfwGetFramebufferSize(m_window, &m_width, &m_height);
	m_swapchainSupportDetails = getSwapChainSupportDetails(m_physicalDevice, m_surface, m_width, m_height);

	m_swapchain = createSwapchain(VK_NULL_HANDLE, m_swapchainSupportDetails, m_logicalDevice, m_surface, m_queueFamilyIndexes);

	// Images TODO
	uint32_t nImages = 0;
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &nImages, nullptr);
	m_images.resize(nImages);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &nImages, m_images.data());
}

std::vector<VkImageView> Window::createImageViews(VkDevice logicalDevice, std::vector<VkImage>& images, SwapChainSupportDetails& swapchainSupportDetails)
{
	std::vector<VkImageView> imageViews(images.size());

	for (int i = imageViews.size() - 1; i >= 0; --i)
	{
		imageViews[i] = createImageView(images[i], swapchainSupportDetails.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	return imageViews;
}

void Window::createRenderPass()
{
	std::vector<VkAttachmentDescription> attachments(2, VkAttachmentDescription());

	VkAttachmentDescription& colorAttachment = attachments[0];
	colorAttachment.format = m_swapchainSupportDetails.surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription& depthAttachment = attachments[1];
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;




	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;




	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create render pass.");
	}
}

void Window::createDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings(2, VkDescriptorSetLayoutBinding());

	VkDescriptorSetLayoutBinding& uboLayoutBinding = bindings[0];
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding& samplerLayoutBinding = bindings[1];
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo = {};
	descriptorSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetCreateInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_logicalDevice, &descriptorSetCreateInfo, nullptr, &m_uboDescriptorSetLayout) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create UBO descriptor set layout.");
	}
}

void Window::createGraphicsPipeline(const char* vertexPath, const char* fragmentPath)
{
	VkShaderModule vertexModule = createShaderModule(vertexPath);
	VkShaderModule fragmentModule = createShaderModule(fragmentPath);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		getCreateShaderPipelineInfo(vertexModule, VK_SHADER_STAGE_VERTEX_BIT),
		getCreateShaderPipelineInfo(fragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT)
	};



	///////////////////////////////
	////// VERTEX ATTRIBUTES //////
	///////////////////////////////

	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapchainSupportDetails.extent.width;
	viewport.height = (float)m_swapchainSupportDetails.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchainSupportDetails.extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 1.0f;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	// SHADOW MAPPING SHIT
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingCreateInfo.minSampleShading = 1.0f;
	multisamplingCreateInfo.pSampleMask = nullptr;
	multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendingCreateInfo.blendConstants[0] = 0.0f;
	colorBlendingCreateInfo.blendConstants[1] = 0.0f;
	colorBlendingCreateInfo.blendConstants[2] = 0.0f;
	colorBlendingCreateInfo.blendConstants[3] = 0.0f;

	VkDynamicState dynamicState = VK_DYNAMIC_STATE_VIEWPORT;
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 1;
	dynamicStateCreateInfo.pDynamicStates = &dynamicState;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_uboDescriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create pipeline layout.");;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;

	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.minDepthBounds = 0.0f;
	depthStencilCreateInfo.maxDepthBounds = 1.0f;

	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilCreateInfo.front = {};
	depthStencilCreateInfo.back = {};

	//////////////////////
	////// PIPELINE //////
	//////////////////////

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = shaderStages.size();
	graphicsPipelineCreateInfo.pStages = shaderStages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.layout = m_pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = m_renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create graphics pipeline.");
	}

	vkDestroyShaderModule(m_logicalDevice, vertexModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragmentModule, nullptr);
}

void Window::createFramebuffers()
{
	m_framebuffers.resize(m_imageViews.size());

	for (int i = m_imageViews.size() - 1; i >= 0; --i)
	{
		std::vector<VkImageView> attachments({ m_imageViews[i], m_depthImageView });

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = m_swapchainSupportDetails.extent.width;
		framebufferCreateInfo.height = m_swapchainSupportDetails.extent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(m_logicalDevice, &framebufferCreateInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to create framebuffer.");
		}
	}
}

void Window::createCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = m_queueFamilyIndexes.graphical;
	commandPoolCreateInfo.flags = 0;

	if (vkCreateCommandPool(m_logicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create command pool.");
	}
}

void Window::createDepthResources()
{

	VkFormat depthFormat = findDepthFormat();

	createImage(
		m_swapchainSupportDetails.extent.width, m_swapchainSupportDetails.extent.height,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_depthImage,
		&m_depthImageMemory
	);

	m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Window::createTextureImage()
{
	Image texture = FileReader::readImage("resources/maggie.png");

	VkDeviceSize textureSize = texture.width * texture.height * 4;
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(textureSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, textureSize, 0, &data);
	memcpy(data, texture.pixels, textureSize);
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	texture.free();

	createImage(
		texture.width, texture.height,
		IMAGE_FORMAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_textureImage, &m_textureImageMemory);

	transitionImageLayout(m_textureImage, IMAGE_FORMAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, m_textureImage, texture.width, texture.height);
	transitionImageLayout(m_textureImage, IMAGE_FORMAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void Window::createTextureImageView()
{
	m_textureImageView = createImageView(m_textureImage, IMAGE_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Window::createTextureSampler()
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;

	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// lower performance
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16;

	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;

	CALL_VK(vkCreateSampler(m_logicalDevice, &samplerCreateInfo, nullptr, &m_textureSampler));
}

void Window::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_vertexBuffer, &m_vertexBufferMemory);

	copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);

}

void Window::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_indexes[0]) * m_indexes.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_indexes.data(), (size_t)bufferSize);
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_indexBuffer, &m_indexBufferMemory);

	copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void Window::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(m_images.size() * 2);
	m_uniformBuffersMemory.resize(m_uniformBuffers.size());

	for (int i = 0; i < m_uniformBuffers.size(); ++i)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_uniformBuffers[i], &m_uniformBuffersMemory[i]);
	}

}

void Window::createDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes(2, VkDescriptorPoolSize());

	VkDescriptorPoolSize& uboPoolSize = poolSizes[0];
	uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboPoolSize.descriptorCount = (uint32_t)m_uniformBuffers.size();

	VkDescriptorPoolSize& samplerPoolSize = poolSizes[1];
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = (uint32_t)m_uniformBuffers.size();

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(m_uniformBuffers.size());
	descriptorPoolCreateInfo.flags = 0;

	if (vkCreateDescriptorPool(m_logicalDevice, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create descriptor pool.");
	}
}

void Window::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(m_uniformBuffers.size(), m_uboDescriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = (uint32_t)layouts.size();
	descriptorSetAllocateInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(layouts.size());
	if (vkAllocateDescriptorSets(m_logicalDevice, &descriptorSetAllocateInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw VulkanException("Failed to allocate descriptor sets.");
	}

	for (int i = m_descriptorSets.size() - 1; i >= 0; --i)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites(2, VkWriteDescriptorSet());


		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = m_uniformBuffers[i];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet& uboDescriptorWrite = descriptorWrites[0];
		uboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboDescriptorWrite.dstSet = m_descriptorSets[i];
		uboDescriptorWrite.dstBinding = 0;
		uboDescriptorWrite.dstArrayElement = 0;
		uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboDescriptorWrite.descriptorCount = 1;
		uboDescriptorWrite.pBufferInfo = &descriptorBufferInfo;
		uboDescriptorWrite.pImageInfo = nullptr;
		uboDescriptorWrite.pTexelBufferView = nullptr;


		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_textureImageView;
		imageInfo.sampler = m_textureSampler;

		VkWriteDescriptorSet& imageDescriptorWrite = descriptorWrites[1];
		imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageDescriptorWrite.dstSet = m_descriptorSets[i];
		imageDescriptorWrite.dstBinding = 1;
		imageDescriptorWrite.dstArrayElement = 0;
		imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescriptorWrite.descriptorCount = 1;
		imageDescriptorWrite.pBufferInfo = nullptr;
		imageDescriptorWrite.pImageInfo = &imageInfo;
		imageDescriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Window::createCommandBuffers()
{
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
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_swapchainSupportDetails.extent;

	// ATTACHMENT ORDER
	std::vector<VkClearValue> clearValues({ { 0.0f, 0.1f, 0.1f, 1.0f }, { 1.0f , 0 } });
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();


	for (int i = m_commandBuffers.size() - 1; i >= 0; --i)
	{
		if (vkBeginCommandBuffer(m_commandBuffers[i], &commandBufferBeginInfo) != VK_SUCCESS)
		{
			throw VulkanException("Failed to begin command buffer.");
		}

		renderPassBeginInfo.framebuffer = m_framebuffers[i];

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[(i * 2)], 0, nullptr);
		vkCmdDrawIndexed(m_commandBuffers[i], (uint32_t)m_indexes.size(), 1, 0, 0, 0);
		
		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[(i * 2) + 1], 0, nullptr);
		vkCmdDrawIndexed(m_commandBuffers[i], (uint32_t)m_indexes.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_commandBuffers[i]);

		if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
		{
			throw VulkanException("Failed to end command buffer.");
		}
	}
}

void Window::createSyncObjects()
{
	////////////////////////
	////// SEMAPHORES //////
	////////////////////////

	m_semaphoresImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
	m_semaphoresRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);

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

	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

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

	if (isPhysicalDeviceSuitable)
	{
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && supportedFeatures.samplerAnisotropy;
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
		if (format.format == IMAGE_FORMAT && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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

uint32_t Window::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeFilter, VkMemoryPropertyFlags memoryPropertyFlags)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if (memoryTypeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
		{
			return i;
		}
	}

	throw VulkanException("Failed to find suitable memory type.");
}

void Window::createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_logicalDevice, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS)
	{
		throw VulkanException("Failed to create buffer");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_logicalDevice, *buffer, &memoryRequirements);


	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(
		m_physicalDevice,
		memoryRequirements.memoryTypeBits,
		propertyFlags
	);

	if (vkAllocateMemory(m_logicalDevice, &memoryAllocateInfo, nullptr, bufferMemory) != VK_SUCCESS)
	{
		throw VulkanException("Failed to allocate buffer memory.");
	}

	vkBindBufferMemory(m_logicalDevice, *buffer, *bufferMemory, 0);

}

void Window::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkImage* image, VkDeviceMemory* deviceMemory)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;

	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	CALL_VK(vkCreateImage(m_logicalDevice, &imageCreateInfo, nullptr, image));

	VkMemoryRequirements imageMemoryRequirements;
	vkGetImageMemoryRequirements(m_logicalDevice, *image, &imageMemoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(
		m_physicalDevice,
		imageMemoryRequirements.memoryTypeBits,
		propertyFlags);

	CALL_VK(vkAllocateMemory(m_logicalDevice, &memoryAllocateInfo, nullptr, deviceMemory));

	vkBindImageMemory(m_logicalDevice, *image, *deviceMemory, 0);
}

void Window::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer transferCommandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommand(transferCommandBuffer);
}

void Window::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
	VkCommandBuffer transferBuffer = beginSingleTimeCommands();

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	VkImageSubresourceLayers& subresource = copyRegion.imageSubresource;
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.baseArrayLayer = 0;
	subresource.layerCount = 1;

	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(transferBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	endSingleTimeCommand(transferBuffer);
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
	physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t)DEVICE_EXTENSIONS.size();

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

VkSwapchainKHR Window::createSwapchain(VkSwapchainKHR oldSwapchain, const SwapChainSupportDetails& swapChainSupportDetails, VkDevice logicalDevice, VkSurfaceKHR surfaceHandle, QueueFamilyIndexes& familyIndexes) const
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
	createInfo.oldSwapchain = oldSwapchain;

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

VkShaderModule Window::createShaderModule(const char* shaderPath)
{
	std::vector<char> shaderData = FileReader::readData(shaderPath);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderData.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderData.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw VulkanException("Couldn't create shader module : " + std::string(shaderPath));
	}

	return shaderModule;
}

VkPipelineShaderStageCreateInfo Window::getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage)
{
	VkPipelineShaderStageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage = shaderStage;
	createInfo.module = shaderModule;
	createInfo.pName = "main";

	return createInfo;
}


///////////////////////////////////////////////////////////////////////
VkCommandBuffer Window::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo  cmdBufferAllocateInfo = {};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandPool = m_commandPool;
	cmdBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	CALL_VK(vkAllocateCommandBuffers(m_logicalDevice, &cmdBufferAllocateInfo, &commandBuffer));

	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	CALL_VK(vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo));

	return commandBuffer;
}

void Window::endSingleTimeCommand(VkCommandBuffer commandBuffer)
{
	CALL_VK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &commandBuffer);
}
///////////////////////////////////////////////////////////////////////

void Window::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;

	VkImageSubresourceRange& subresourceRange = barrier.subresourceRange;
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;


	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw VulkanException("unsupported layout transition!");
	}


	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommand(commandBuffer);
}

VkImageView Window::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;

	VkImageSubresourceRange& subresourceRange = viewCreateInfo.subresourceRange;
	subresourceRange.aspectMask = aspectFlags;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	VkImageView imageView;
	CALL_VK(vkCreateImageView(m_logicalDevice, &viewCreateInfo, nullptr, &imageView));

	return imageView;
}


VkFormat Window::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}

		throw VulkanException("Failed to find supported format.");
	}
}

VkFormat Window::findDepthFormat()
{
	return findSupportedFormat(
		{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool Window::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}



void Window::cleanupSwapChain()
{
	vkDestroyImageView(m_logicalDevice, m_depthImageView, nullptr);
	vkDestroyImage(m_logicalDevice, m_depthImage, nullptr);
	vkFreeMemory(m_logicalDevice, m_depthImageMemory, nullptr);


	for (VkFramebuffer framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

	for (VkImageView imageView : m_imageViews)
	{
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);

	for (int i = 0; i < m_uniformBuffers.size(); ++i)
	{
		vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_logicalDevice, m_uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
}

void Window::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_logicalDevice);

	cleanupSwapChain();
	createSwapChain();
	m_imageViews = createImageViews(m_logicalDevice, m_images, m_swapchainSupportDetails);
	createRenderPass();
	createGraphicsPipeline("shaders/bin/triangle.vert.spv", "shaders/bin/triangle.frag.spv");

	createDepthResources();

	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	createCommandBuffers();
}
