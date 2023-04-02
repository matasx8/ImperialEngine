#pragma once
#include "Utils/NonCopyable.h"
#include "backend/graphics/Pipeline.h"
#include "backend/graphics/RenderPass/RenderPass.h"
#include <optional>

namespace imp
{
	class PipelineManager : NonCopyable
	{
	public: 
		PipelineManager();

		Pipeline CreatePipeline(VkDevice device, const RenderPass& rp, const PipelineConfig& config);
		const Pipeline& GetOrCreatePipeline(VkDevice device, const RenderPass& rp, const PipelineConfig& config);
		const Pipeline& GetComputePipeline(const ComputePipelineConfig& config);

		void CreateComputePipeline(VkDevice device, const ComputePipelineConfig& config);

		void Destroy(VkDevice deviec);

	private:

		VkPipelineShaderStageCreateInfo MakeShaderStageCI(VkShaderModule module, VkShaderStageFlagBits stage) const;
		VkVertexInputBindingDescription MakeVertexBindingDesc() const;
		std::vector<VkVertexInputAttributeDescription> MakeVertexAttrDescs() const;
		VkVertexInputAttributeDescription MakeVertexAttrDesc(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) const;
		VkPipelineVertexInputStateCreateInfo MakeVertexInputStateCI(const auto& vertInputBindingDesc, const auto& vertInputAttrDesc) const;
		VkPipelineInputAssemblyStateCreateInfo MakeInputAssemblyCI() const;
		VkPipelineViewportStateCreateInfo MakeViewportStateCI(const auto& viewport, const auto& scissor) const;
		VkPipelineRasterizationStateCreateInfo MakeRasterizationSateCI() const;
		VkPipelineMultisampleStateCreateInfo MakeMSAAStateCI(const RenderPass& rp) const;
		VkPipelineColorBlendStateCreateInfo MakeColorBlendStateCI(const auto& blendAttState) const;
		VkPipelineDepthStencilStateCreateInfo MakeDepthStencilStateCI() const;
		VkPipelineLayoutCreateInfo MakePipelineLayoutCI(const VkPushConstantRange* pushRange, const std::vector<VkDescriptorSetLayout>& layouts) const;
		VkPipelineLayout MakePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& ci) const;
		VkGraphicsPipelineCreateInfo MakePipelineCI(const VkPipelineShaderStageCreateInfo* shaderStages, uint32_t stageCount, const VkPipelineVertexInputStateCreateInfo* vertexInputCreateInfo, const
			VkPipelineInputAssemblyStateCreateInfo* inputAssembly, const VkPipelineViewportStateCreateInfo* viewportStateCreateInfo, const
			VkPipelineDynamicStateCreateInfo* dynamicState, const VkPipelineRasterizationStateCreateInfo* rasterizerCreateInfo, const
			VkPipelineMultisampleStateCreateInfo* multisamplingCreateInfo, const VkPipelineColorBlendStateCreateInfo* colourBlendingCreateInfo, const
			VkPipelineDepthStencilStateCreateInfo* depthStencilCreateInfo, const VkPipelineLayout pipelineLayout, const
			VkRenderPass renderPass, VkPipelineCreateFlags flags) const;
		VkPipeline MakePipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& ci) const;

		std::unordered_map<PipelineConfig, Pipeline, PipelineConfigHash> m_PipelineMap;
		std::unordered_map<ComputePipelineConfig, Pipeline, ComputePipelineConfigHash> m_ComputePipelineMap;
	};
}