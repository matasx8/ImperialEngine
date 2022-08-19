#pragma once
#include "backend/VulkanResource.h"
#include <string>

namespace imp
{
	class VulkanShader : VulkanResource
	{
	public:
		VulkanShader();
		VulkanShader(const std::string& name, VkShaderModule shaderModule);

		const std::string& GetName() const;
		VkShaderModule GetShaderModule() const;

		void Destroy(VkDevice device) override;
	private:
		std::string m_Name;
		VkShaderModule m_ShaderModule;
	};
}
