#include "Shader.h"

#include "FileReader.h"
#include "VulkanException.h"

ShaderBuilder::ShaderBuilder(VkDevice device) :
	m_device(device)
{
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderBuilder::createPipeline(const char* vertexPath, const char* fragmentPath)
{
	VkShaderModule vertexModule = createShaderModule(vertexPath);
	VkShaderModule fragmentModule = createShaderModule(fragmentPath);
	
	std::vector<VkPipelineShaderStageCreateInfo> pipelineCreateInfos = {
		getCreateShaderPipelineInfo(vertexModule, VK_SHADER_STAGE_VERTEX_BIT),
		getCreateShaderPipelineInfo(fragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	vkDestroyShaderModule(m_device, vertexModule, nullptr);
	vkDestroyShaderModule(m_device, fragmentModule, nullptr);
	
	return pipelineCreateInfos;
}

VkShaderModule ShaderBuilder::createShaderModule(const char* shaderPath)
{
	std::vector<char> shaderData = FileReader::read(shaderPath);

	VkShaderModuleCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(VkShaderModuleCreateInfo));

	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderData.size();
	createInfo.pCode = (uint32_t*) shaderData.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw VulkanException("Couldn't create shader module : " + std::string(shaderPath));
	}

	return shaderModule;
}

VkPipelineShaderStageCreateInfo ShaderBuilder::getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage)
{
	VkPipelineShaderStageCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));

	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage = shaderStage;
	createInfo.module = shaderModule;
	createInfo.pName = "main";

	return createInfo;
}
