#pragma once
#include <vector>
#include <string>

#include "Image.h"

class FileReader
{
public:
	static std::vector<char> readData(const char* relativePath);
	static Image readImage(const char* imagePath);

private:
	FileReader();

	static const std::string ROOT_PATH;
};
