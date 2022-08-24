#include "VulkanShaderManager.h"
#include <optional>

imp::VulkanShaderManager::VulkanShaderManager()
	: m_ShaderMap()
{
}

imp::VulkanShader imp::VulkanShaderManager::GetShader(const std::string& shaderName)
{
	const auto shader = m_ShaderMap.find(shaderName);
	if (shader != m_ShaderMap.end())
		return shader->second;
	return VulkanShader();	// TODO: return default/error shader
}

void imp::VulkanShaderManager::CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req)
{
	const auto vertexShaderModule = CreateShaderModule(device, *req.vertexSpv.get());
	const auto fragmentShaderModule = CreateShaderModule(device, *req.fragmentSpv.get());

	const VulkanShader vertexShader(vertexShaderModule);
	const VulkanShader fragmentShader(fragmentShaderModule);
	m_ShaderMap[req.shaderName + ".vert"] = vertexShader;
	m_ShaderMap[req.shaderName + ".frag"] = fragmentShader;
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
