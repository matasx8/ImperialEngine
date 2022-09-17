#pragma once
#include "backend/VulkanResource.h"

namespace imp
{
	struct PipelineConfig
	{
		VkShaderModule vertModule;
		VkShaderModule fragModule;
		VkDescriptorSetLayout descriptorSetLayout;
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