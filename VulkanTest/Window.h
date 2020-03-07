#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "DeviceBuilder.h"
#include "PipelineBuilder.h"

class Window
{
public:
	Window(
		int width = 800,
		int heigth = 600);

	~Window();

	void init();
	void destroy();

	bool isOpen();
	void draw();

	VkDevice getDevice();

private:
	int m_width;
	int m_height;

	GLFWwindow* m_window;
	VkSurfaceKHR m_surface;

	VkInstance m_instance;
	
	DeviceConfigurations m_deviceConfigurations;
	PipelineConfigurations m_pipelineConfigurations;

	std::vector<VkFramebuffer> m_framebuffers;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_semaphoresImageAvailable;
	std::vector<VkSemaphore> m_semaphoresRenderFinished;

	uint32_t m_currentFrameIndex;

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif // !NDEBUG
};
