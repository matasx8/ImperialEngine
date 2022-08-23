#pragma once
#include "Utils/NonCopyable.h"
#include "backend/EngineCommandResources.h"
#include "backend/graphics/VulkanShader.h"
#include <unordered_map>
#include <string>

namespace imp
{
	class VulkanShaderManager : NonCopyable
	{
	public:
		VulkanShaderManager();

		VulkanShader GetShader(const std::string& shaderName);

		void CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req);

	private:

		VkShaderModule CreateShaderModule(VkDevice device, const std::string& shaderCode);
		std::unordered_map<std::string, VulkanShader> m_ShaderMap;
	};
}