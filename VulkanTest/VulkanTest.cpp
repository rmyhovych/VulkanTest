#include "glm/glm.hpp"

#include <iostream>
#include <vector>

#include "Window.h"

int main()
{
	Window window(1000, 600);
	window.init();

	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	while (window.isOpen()) 
	{
		window.swapBuffers();
	}

	window.destroy();
	return 0;
}
