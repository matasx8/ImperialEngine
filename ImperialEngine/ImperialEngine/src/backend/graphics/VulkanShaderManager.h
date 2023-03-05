#pragma once
#include "Utils/NonCopyable.h"
#include "backend/VariousTypeDefinitions.h"
#include "backend/graphics/VulkanShader.h"
#include "backend/VulkanBuffer.h"
#include "frontend/EngineSettings.h"
#include <extern/GLM/mat4x4.hpp>
#include <extern/GLM/vec4.hpp>
#include <unordered_map>
#include <string>
#include <array>

namespace imp
{
	inline constexpr uint32_t kBindingCount					= 5;
	inline constexpr uint32_t kMaxMaterialCount				= 128;
	inline constexpr uint32_t kMaxDrawCount					= 1'048'000; //Should be upper bound, lets see what happens with 2
	inline constexpr uint32_t kMaxMeshCount					= kMaxDrawCount;
	inline constexpr uint32_t kGlobalBufferBindingSlot		= 0;
	inline constexpr uint32_t kGlobalBufferBindCount		= 1;
	inline constexpr uint32_t kVertexBufferBindingSlot		= kGlobalBufferBindingSlot + kGlobalBufferBindCount;
	inline constexpr uint32_t kVertexBufferBindingCount		= 1;
	inline constexpr uint32_t kMaterialBufferBindingSlot	= kVertexBufferBindingSlot + kVertexBufferBindingCount;
	inline constexpr uint32_t kMaterialBufferBindCount		= kMaxMaterialCount;
	inline constexpr uint32_t kDrawDataIndicesBindingSlot	= kMaterialBufferBindingSlot + kMaterialBufferBindCount;
	inline constexpr uint32_t kDrawDataIndicesBindCount		= 1;
	inline constexpr uint32_t kDrawDataBufferBindingSlot	= kDrawDataIndicesBindingSlot + kDrawDataIndicesBindCount;
	inline constexpr uint32_t kDefaultMaterialIndex			= 0;

	inline constexpr uint32_t kComputeBindingCount			= 4;

	struct GlobalData
	{
		glm::mat4x4 ViewProjection;
	};

	struct MaterialData
	{
		glm::vec4 color;
	};

	struct alignas(16) ShaderDrawData
	{
		glm::mat4x4 transform;
		uint32_t materialIndex;
	};

	class VulkanMemory;
	class PipelineManager;
	struct MemoryProps;
	struct DrawDataSingle;

	class VulkanShaderManager : NonCopyable
	{
	public:
		VulkanShaderManager();

		void Initialize(VkDevice device, VulkanMemory& memory, const EngineGraphicsSettings& settings, const MemoryProps& memProps, VulkanBuffer& drawCommands, VulkanBuffer& vertices);

		VulkanShader GetShader(const std::string& shaderName) const;
		VkDescriptorSet GetDescriptorSet(uint32_t idx) const;
		VkDescriptorSet GetComputeDescriptorSet(uint32_t idx) const;
		VkDescriptorSetLayout GetDescriptorSetLayout() const;
		VkDescriptorSetLayout GetComputeDescriptorSetLayout() const;
		VulkanBuffer& GetGlobalDataBuffer(uint32_t idx);
		VulkanBuffer& GetDrawDataBuffers(uint32_t idx);
		VulkanBuffer& GetDrawCommandBuffer();
		VulkanBuffer& GetDrawDataIndicesBuffer();
		VulkanBuffer& GetMeshDataBuffer();
		VulkanBuffer& GetDrawCommandCountBuffer();

		void CreateVulkanShaderSet(VkDevice device, const MaterialCreationRequest& req);
		void CreateComputePrograms(VkDevice device, PipelineManager& pipeManager, const ComputeProgramCreationRequest& req);
		void UpdateGlobalData(VkDevice device, uint32_t descriptorSetIdx, const GlobalData& data);
		void UpdateDrawData(VkDevice device, uint32_t descriptorSetIdx, const std::vector<DrawDataSingle> drawData);

	private:

		VkDescriptorPool CreateDescriptorPool(VkDevice device);
		VkShaderModule CreateShaderModule(VkDevice device, const std::string& shaderCode);
		void CreateMegaDescriptorSets(VkDevice device);
		void CreateMegaDescriptorSetLayout(VkDevice device);
		void CreateComputeDescriptorSetLayout(VkDevice device);
		VkDescriptorSetLayoutBinding CreateDescriptorBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type, VkShaderStageFlags stageFlags);

		void WriteUpdateDescriptorSets(VkDevice device, VkDescriptorSet* dSets, VkDescriptorType type, std::array<VulkanBuffer, 3>& buffers, size_t descriptorDataSize, uint32_t bindSlot, uint32_t descriptorCount, uint32_t dSetCount);
		void WriteUpdateDescriptorSetsSingleBuffer(VkDevice device, VkDescriptorSet* dSets, VkDescriptorType type, VulkanBuffer& buffer, size_t descriptorDataSize, uint32_t bindSlot, uint32_t descriptorCount, uint32_t dSetCount);
		void UpdateDescriptorData(VkDevice device, VulkanBuffer& buffer, size_t size, uint32_t offset, const void* data);

		void CreateDefaultMaterial(VkDevice device);


		std::unordered_map<std::string, VulkanShader> m_ShaderMap;
		VkDescriptorPool m_DescriptorPool;

		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_GlobalBuffers;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_MaterialDataBuffers;
		VulkanBuffer m_DrawDataIndices;
		std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_DrawDataBuffers;

		// compute
		//std::array<VulkanBuffer, kEngineSwapchainExclusiveMax - 1> m_DrawCommands;
		VulkanBuffer m_DrawCommands; // out custom draw commands
		VulkanBuffer m_MeshData;
		VulkanBuffer m_DrawCommandCount;

		std::array<VkDescriptorSet, kEngineSwapchainExclusiveMax - 1> m_DescriptorSets;
		std::array<VkDescriptorSet, kEngineSwapchainExclusiveMax - 1> m_ComputeDescriptorSets;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorSetLayout m_ComputeDescriptorSetLayout;
	};
}