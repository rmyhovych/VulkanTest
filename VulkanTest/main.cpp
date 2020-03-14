#include <iostream>
#include <vector>

#include "Window.h"

int main()
{
	Window window(800, 500);
	window.init();

	while (window.isOpen()) 
	{
		window.draw();
	}

	vkDeviceWaitIdle(window.getDevice());
	window.destroy();
	return 0;
}
