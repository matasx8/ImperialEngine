#pragma once
#include "Utils/NonCopyable.h"
#include "backend/EngineCommandResources.h"
#include "backend/graphics/VulkanShader.h"
#include "backend/VulkanBuffer.h"
#include "frontend/EngineSettings.h"
#include <extern/GLM/mat4x4.hpp>
#include <extern/GLM/vec4.hpp>
#include <unordered_map>
#include <string>

namespace imp
{
	inline constexpr uint32_t kBindingCount					= 3;
	inline constexpr uint32_t kMaxMaterialCount				= 128;
	inline constexpr uint32_t kMaxDrawCount					= 2;	//1'000'000; Should be upper bound, lets see what happens with 2
	inline constexpr uint32_t kGlobalBufferBindingSlot		= 0;
	inline constexpr uint32_t kGlobalBufferBindCount		= 1;
	inline constexpr uint32_t kMaterialBufferBindingSlot	= kGlobalBufferBindingSlot + kGlobalBufferBindCount;
	inline constexpr uint32_t kMaterialBufferBindCount		= kMaxMaterialCount;
	inline constexpr uint32_t kDrawDataBufferBindingSlot	= kMaterialBufferBindingSlot + kMaterialBufferBindCount;
	inline constexpr uint32_t kDefaultMaterialIndex			= 0;

	struct ShaderDescription
	{

	};

	struct GlobalData
	{
		glm::mat4x4 ViewProjection;
	};

	struct MaterialData
	{
		glm::vec4 color;
	};

	struct ShaderDrawData
	{
		glm::mat4x4 transform;
		uint32_t materialIndex;
		uint32_t isEnabled; // could be used for CS to cull or something like that
		uint32_t padding[2];
	};

	class VulkanMemory;
	struct MemoryProps;
	class DrawDataSingle;

	class VulkanShaderManager : NonCopyable
	{
	public:
		VulkanShaderManager();

		void Initialize(VkDevice device, VulkanMemory& memory, const EngineGraphicsSettings& settings, const MemoryProps& memProps);

		VulkanShader GetShader(const std::string& shaderName) const;
		VkDescriptorSet GetDescriptorSet(uint32_t idx) const;
		VkDescriptorSetLayout GetDescriptorSetLayout() const;

		void CreateVulkanShaderSet(VkDevice device, const CmdRsc::MaterialCreationRequest& req);
		void UpdateGlobalData(VkDevice device, uint32_t descriptorSetIdx, const GlobalData& data);
		void UpdateDrawData(VkDevice device, uint32_t descriptorSetIdx, const std::vector<DrawDataSingle> drawData);

		void RegisterDraws(VkDevice device, uint32_t numDraws);

		// new material stuff


	private:

		VkDescriptorPool CreateDescriptorPool(VkDevice device);
		VkShaderModule CreateShaderModule(VkDevice device, const std::string& shaderCode);
		void CreateMegaDescriptorSets(VkDevice device);
		void CreateMegaDescriptorSetLayout(VkDevice device);
		VkDescriptorSetLayoutBinding CreateDescriptorBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkShaderStageFlags stageFlags);

		uint32_t WriteUpdateDescriptorSets(VkDevice device, VkDescriptorType type, auto& buffers, size_t size, int index, uint32_t bindSlot);
		void UpdateDescriptorData(VkDevice device, VulkanBuffer& buffer, size_t size, uint32_t offset, const void* data);

		void CreateDefaultMaterial(VkDevice device);


		std::unordered_map<std::string, VulkanShader> m_ShaderMap;
		VkDescriptorPool m_DescriptorPool;
		//	not sure if I need triple buffering. Is double buffering enough?
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_GlobalBuffers;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_MaterialDataBuffers;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_DrawDataBuffers;

		std::array<VkDescriptorSet, kEngineSwapchainExclusiveMax - 1> m_DescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout;
	};
}