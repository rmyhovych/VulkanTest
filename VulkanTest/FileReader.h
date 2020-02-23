#pragma once
#include <vector>
#include <string>

class FileReader
{
public:
	static std::vector<char> read(const char* relativePath);

private:
	FileReader();

	static const std::string ROOT_PATH;
};
