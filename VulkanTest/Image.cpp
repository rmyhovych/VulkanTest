#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(const char* filepath)
{
	pixels = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
}

void Image::free()
{
	stbi_image_free(pixels);
}
