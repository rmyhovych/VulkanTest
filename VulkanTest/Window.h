#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "PipelineBuilder.h"

class Window
{
public:
	Window(
		int width = 800,
		int heigth = 600);

	void init();
	void destroy();

	bool isOpen();
	void draw();

	VkDevice getDevice();

	void setResized();

private:
	void initSwapChain();
	void destroySwapChain();

	void recreateSwapChain();

private:
	// DEVICE
	bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle);
	VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndexes& familyIndexes);

	// QUEUES
	QueueFamilyIndexes getQueueFamilyIndexes(VkPhysicalDevice physicalDevice);

	bool getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index);
	bool getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t* index, uint32_t priorityIndex);
	void setQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo, uint32_t index, float priority);


	// SWAPCHAIN
	VkSwapchainKHR createSwapchain(VkSwapchainKHR oldSwapchain, const SwapChainSupportDetails& swapChainSupportDetails, VkDevice logicalDevice, VkSurfaceKHR surfaceHandle, QueueFamilyIndexes& familyIndexes) const;

	SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t width, uint32_t height);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& swapChainFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& swapChainCapabilities, uint32_t width, uint32_t height);

	// IMAGE
	std::vector<VkImageView> createImageViews(VkDevice logicalDevice, std::vector<VkImage>& images, SwapChainSupportDetails& swapchainSupportDetails) const;



	// MEMORY SHIT
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeFilter, VkMemoryPropertyFlags memoryPropertyFlags);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);

	void createVertexBuffer();


private:
	int m_width;
	int m_height;

	GLFWwindow* m_window;
	VkSurfaceKHR m_surface;

	VkInstance m_instance;
	
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;

	QueueFamilyIndexes m_queueFamilyIndexes;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	SwapChainSupportDetails m_swapchainSupportDetails;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;

	PipelineConfigurations m_pipelineConfigurations;

	std::vector<VkFramebuffer> m_framebuffers;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_semaphoresImageAvailable;
	std::vector<VkSemaphore> m_semaphoresRenderFinished;

	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;

	uint32_t m_currentFrameIndex;
	bool m_framebufferResized;


	std::vector<Vertex> m_vertices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;


#ifndef NDEBUG
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif // !NDEBUG
};
