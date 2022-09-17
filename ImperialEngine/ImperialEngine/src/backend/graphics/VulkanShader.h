#pragma once
#include "backend/VulkanResource.h"
#include <string>

namespace imp
{
	class VulkanShader : VulkanResource
	{
	public:
		VulkanShader();
		VulkanShader(VkShaderModule shaderModule);

		VkShaderModule GetShaderModule() const;

		void Destroy(VkDevice device) override;
	private:
		VkShaderModule m_ShaderModule;
	};
}

