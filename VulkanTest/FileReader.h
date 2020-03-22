#pragma once
#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Image
{
	int width;
	int height;
	int channels;
	stbi_uc* pixels;

	void free()
	{
		stbi_image_free(pixels);
	}
};

class FileReader
{
public:
	static std::vector<char> readData(const char* relativePath);
	static Image readImage(const char* imagePath);

private:
	FileReader();

	static const std::string ROOT_PATH;
};
