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

		void CreatePipeline(const RenderPassBase& rp, const PipelineConfig& config);
		const Pipeline& GetOrCreatePipeline(const RenderPassBase& rp, const PipelineConfig& config);
		auto GetPipeline() const;

		void Destroy(VkDevice device);

	private:

		VkPipelineShaderStageCreateInfo MakeShaderStageCI(VkShaderModule module, VkShaderStageFlagBits stage) const;
		VkVertexInputBindingDescription MakeVertexBindingDesc() const;
		std::vector<VkVertexInputAttributeDescription> MakeVertexAttrDescs() const;
		VkVertexInputAttributeDescription MakeVertexAttrDesc(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset) const;
		VkPipelineVertexInputStateCreateInfo MakeVertexInputStateCI() const;
		VkPipelineInputAssemblyStateCreateInfo MakeInputAssemblyCI() const;
		// TODO: have an O(1) get container of pipelines
		Pipeline m_TemporarySinglePipeline;
	};
}