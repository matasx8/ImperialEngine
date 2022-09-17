#include "Pipeline.h"

imp::Pipeline::Pipeline()
	: m_Pipeline(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE)
{
}

imp::Pipeline::Pipeline(VkPipeline pipeline, VkPipelineLayout layout)
	: m_Pipeline(pipeline), m_PipelineLayout(layout)
{
}

VkPipeline imp::Pipeline::GetPipeline() const
{
	return m_Pipeline;
}

VkPipelineLayout imp::Pipeline::GetPipelineLayout() const
{
	return m_PipelineLayout;
}

void imp::Pipeline::Destroy(VkDevice device)
{
	vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
	vkDestroyPipeline(device, m_Pipeline, nullptr);
}
