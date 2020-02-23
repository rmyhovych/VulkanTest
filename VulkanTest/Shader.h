#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class ShaderBuilder
{
public:
	ShaderBuilder(VkDevice device);
	
	std::vector<VkPipelineShaderStageCreateInfo> createPipeline(const char* vertexPath, const char* fragmentPath);

private:
	VkShaderModule createShaderModule(const char* shaderPath);
	VkPipelineShaderStageCreateInfo getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage);

private:
	VkDevice m_device;
};

