#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
	void swapBuffers();

private:
	int m_width;
	int m_height;

	GLFWwindow* m_window;
	VkInstance m_instance;
};
