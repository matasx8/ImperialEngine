#include "VulkanShaderManager.h"

imp::VulkanShaderManager::VulkanShaderManager()
	: m_ShaderSet()
{
}

void imp::VulkanShaderManager::CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req)
{
	const auto vertexShaderModule = CreateShaderModule(device, *req.vertexSpv.get());
	const auto fragmentShaderModule = CreateShaderModule(device, *req.fragmentSpv.get());

	const VulkanShader vertexShader(req.shaderName + ".vert", vertexShaderModule);
	const VulkanShader fragmentShader(req.shaderName + ".frag", fragmentShaderModule);
	m_ShaderSet.insert(vertexShader);
	m_ShaderSet.insert(fragmentShader);
}

VkShaderModule imp::VulkanShaderManager::CreateShaderModule(VkDevice device, const std::string& shaderCode)
{
	VkShaderModuleCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	VkShaderModule shaderModule;
	auto res = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
	assert(res == VK_SUCCESS);
	return shaderModule;
}
