#pragma once
#include "backend/VulkanResource.h"
#include "extern/XXHASH/xxhash.h"
#include <compare>

namespace imp
{
	struct PipelineConfig
	{
		VkShaderModule vertModule;
		VkShaderModule fragModule;
		VkDescriptorSetLayout descriptorSetLayout;
		auto operator<=>(const PipelineConfig&) const = default;
	};

	struct ComputePipelineConfig
	{
		VkShaderModule computeModule;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSetLayout descriptorSetLayout2;
		auto operator<=>(const ComputePipelineConfig&) const = default;
	};

	struct ComputePipelineConfigHash
	{
		size_t operator()(const ComputePipelineConfig& config) const
		{
			return XXH64(&config, sizeof(ComputePipelineConfig), 0);
		}
	};

	struct PipelineConfigHash
	{
		size_t operator()(const PipelineConfig& config) const
		{
			return XXH64(&config, sizeof(PipelineConfig), 0);
		}
	};
	class Pipeline : VulkanResource
	{
	public:
		Pipeline();
		Pipeline(VkPipeline pipeline, VkPipelineLayout layout);

		VkPipeline GetPipeline() const;
		VkPipelineLayout GetPipelineLayout() const;

		void Destroy(VkDevice device) override;
	private:

		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;
	};
}