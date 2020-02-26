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
		if (graphicsPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(device, graphicsPipeline, nullptr);
			graphicsPipeline = VK_NULL_HANDLE;
		}
		if (pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			pipelineLayout = VK_NULL_HANDLE;
		}
		if (renderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(device, renderPass, nullptr);
			renderPass = VK_NULL_HANDLE;
		}
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

