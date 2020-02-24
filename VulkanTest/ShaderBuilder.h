#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "DeviceConfigurations.h"

class ShaderBuilder
{
public:
	ShaderBuilder(DeviceConfigurations& deviceConfigurations);
	
	void createGraphicsPipeline(const char* vertexPath, const char* fragmentPath);

private:
	VkShaderModule createShaderModule(const char* shaderPath);
	VkPipelineShaderStageCreateInfo getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage);

private:
	DeviceConfigurations& m_deviceConfigurations;
};

