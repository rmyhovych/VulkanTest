#include "glm/glm.hpp"

#include <iostream>
#include <vector>

#include "Window.h"

int main()
{
	Window window(800, 500);
	window.init();

	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	while (window.isOpen()) 
	{
		window.draw();
	}

	vkDeviceWaitIdle(window.getDevice());
	window.destroy();
	return 0;
}
