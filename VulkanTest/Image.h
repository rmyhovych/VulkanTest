#pragma once

#include <stb_image.h>
#include "vulkan/vulkan.h"

class Image
{
public:
	Image(const char* filepath);
	void free();

	int width;
	int height;
	int channels;
	stbi_uc* pixels;
};

