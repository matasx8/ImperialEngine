#pragma once
#include "Utils/NonCopyable.h"
#include "backend/EngineCommandResources.h"
#include "backend/graphics/VulkanShader.h"
#include <unordered_set>

namespace imp
{
	class VulkanShaderManager : NonCopyable
	{
	public:
		VulkanShaderManager();

		void CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req);

	private:

		VkShaderModule CreateShaderModule(VkDevice device, const std::string& shaderCode);
		std::unordered_set<VulkanShader> m_ShaderSet;
	};
}