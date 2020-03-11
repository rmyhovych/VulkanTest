#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

struct QueueFamilyIndexes
{
	uint32_t graphical;
	uint32_t present;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};


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
	PipelineBuilder(VkDevice logicalDevice, SwapChainSupportDetails& swapchainSupportDetails);

	PipelineConfigurations createGraphicsPipeline(const char* vertexPath, const char* fragmentPath);

private:
	VkShaderModule createShaderModule(const char* shaderPath);
	VkPipelineShaderStageCreateInfo getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage);

private:
	VkDevice m_logicalDevice;
	SwapChainSupportDetails& m_swapchainSupportDetails;
};

