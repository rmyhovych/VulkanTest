#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "DeviceConfigurations.h"

struct PipelineConfigurations
{
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;

	void destroy(VkDevice device)
	{
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
	}
};

class PipelineBuilder
{
public:
	PipelineBuilder(DeviceConfigurations& deviceConfigurations);
	
	PipelineConfigurations createGraphicsPipeline(const char* vertexPath, const char* fragmentPath);

private:
	VkShaderModule createShaderModule(const char* shaderPath);
	VkPipelineShaderStageCreateInfo getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage);

private:
	DeviceConfigurations& m_deviceConfigurations;
};

