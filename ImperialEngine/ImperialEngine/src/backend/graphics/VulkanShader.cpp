#include "VulkanShader.h"

imp::VulkanShader::VulkanShader()
	: m_ShaderModule(VK_NULL_HANDLE)
{
}

imp::VulkanShader::VulkanShader(VkShaderModule shaderModule)
	: m_ShaderModule(shaderModule)
{
}

VkShaderModule imp::VulkanShader::GetShaderModule() const
{
	return m_ShaderModule;
}

void imp::VulkanShader::Destroy(VkDevice device)
{
	vkDestroyShaderModule(device, m_ShaderModule, nullptr);
}
