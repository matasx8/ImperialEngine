#include "VulkanShader.h"

imp::VulkanShader::VulkanShader()
	: m_Name("Invalid"), m_ShaderModule(VK_NULL_HANDLE)
{
}

imp::VulkanShader::VulkanShader(const std::string& name, VkShaderModule shaderModule)
	: m_Name(name), m_ShaderModule(shaderModule)
{
}

const std::string& imp::VulkanShader::GetName() const
{
	return m_Name;
}

VkShaderModule imp::VulkanShader::GetShaderModule() const
{
	return m_ShaderModule;
}

void imp::VulkanShader::Destroy(VkDevice device)
{
	vkDestroyShaderModule(device, m_ShaderModule, nullptr);
}
