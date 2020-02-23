#pragma once

#include <exception>
#include <string>

class VulkanException : 
	public std::exception
{
public:
	VulkanException(const char* message) :
		std::exception(message)
	{}

	VulkanException(std::string message) :
		std::exception(message.c_str())
	{}
};
