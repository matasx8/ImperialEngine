#pragma once
#include "Utils/NonCopyable.h"
#include "backend/EngineCommandResources.h"
#include "backend/graphics/VulkanShader.h"
#include "frontend/EngineSettings.h"
#include <unordered_map>
#include <string>

namespace imp
{
	struct ShaderDescription
	{

	};

	class VulkanMemory;

	class VulkanShaderManager : NonCopyable
	{
	public:
		VulkanShaderManager();

		void Initialize(VkDevice device, VulkanMemory& memory, EngineGraphicsSettings& settings);

		VulkanShader GetShader(const std::string& shaderName);

		void CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req);

		// new material stuff


	private:

		VkShaderModule CreateShaderModule(VkDevice device, const std::string& shaderCode);
		std::unordered_map<std::string, VulkanShader> m_ShaderMap;
	};
}