
#include <exception>

class VulkanException : 
	public std::exception
{
public:
	VulkanException(const char* message) :
		std::exception(message)
	{}
};
