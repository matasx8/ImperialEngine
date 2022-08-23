#pragma once
#include "Utils/NonCopyable.h"
#include "backend/graphics/Pipeline.h"
#include "backend/graphics/RenderPassBase.h"
#include <optional>

namespace imp
{
	class PipelineManager : NonCopyable
	{
	public: 
		PipelineManager();
		void Initialize();

		void CreatePipeline(VkDevice device, const RenderPassBase& rp, const PipelineConfig& config);
		const Pipeline& GetOrCreatePipeline(VkDevice device, const RenderPassBase& rp, const PipelineConfig& config);
		auto GetPipeline() const;

		void Destroy(VkDevice device);

	private:

		VkPipelineShaderStageCreateInfo MakeShaderStageCI(VkShaderModule module, VkShaderStageFlagBits stage) const;
		VkVertexInputBindingDescription MakeVertexBindingDesc() const;
		std::vector<VkVertexInputAttributeDescription> MakeVertexAttrDescs() const;
		VkVertexInputAttributeDescription MakeVertexAttrDesc(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) const;
		VkPipelineVertexInputStateCreateInfo MakeVertexInputStateCI() const;
		VkPipelineInputAssemblyStateCreateInfo MakeInputAssemblyCI() const;
		VkPipelineViewportStateCreateInfo MakeViewportStateCI(const RenderPassBase& rp) const;
		VkPipelineRasterizationStateCreateInfo MakeRasterizationSateCI() const;
		VkPipelineMultisampleStateCreateInfo MakeMSAAStateCI(const RenderPassBase& rp) const;
		VkPipelineColorBlendStateCreateInfo MakeColorBlendStateCI() const;
		VkPipelineDepthStencilStateCreateInfo MakeDepthStencilStateCI() const;
		VkPipelineLayoutCreateInfo MakePipelineLayoutCI(const RenderPassBase& rp) const;
		VkPipelineLayout MakePipelineLayout(VkDevice device, const RenderPassBase& rp) const;
		VkGraphicsPipelineCreateInfo MakePipelineCI(const VkPipelineShaderStageCreateInfo* shaderStages, const VkPipelineVertexInputStateCreateInfo* vertexInputCreateInfo, const
			VkPipelineInputAssemblyStateCreateInfo* inputAssembly, const VkPipelineViewportStateCreateInfo* viewportStateCreateInfo, const
			VkPipelineDynamicStateCreateInfo* dynamicState, const VkPipelineRasterizationStateCreateInfo* rasterizerCreateInfo, const
			VkPipelineMultisampleStateCreateInfo* multisamplingCreateInfo, const VkPipelineColorBlendStateCreateInfo* colourBlendingCreateInfo, const
			VkPipelineDepthStencilStateCreateInfo* depthStencilCreateInfo, const VkPipelineLayout pipelineLayout, const
			VkRenderPass renderPass, VkPipelineCreateFlags flags) const;
		VkPipeline MakePipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& ci) const;
		// TODO: have an O(1) get container of pipelines
		Pipeline m_TemporarySinglePipeline;
	};
}