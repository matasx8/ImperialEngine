#include "PipelineManager.h"

namespace imp
{
	PipelineManager::PipelineManager()
		: m_TemporarySinglePipeline()
	{
	}

	void PipelineManager::Initialize()
	{
	}

	void PipelineManager::CreatePipeline(const RenderPassBase& rp)
	{
		// finish here..
	}

	const Pipeline& PipelineManager::GetOrCreatePipeline(const RenderPassBase& rp)
	{
		if (m_TemporarySinglePipeline.GetPipeline() == VK_NULL_HANDLE)
			CreatePipeline(rp);
		return m_TemporarySinglePipeline;
	}

	auto PipelineManager::GetPipeline() const
	{	// trying out modern c++ magic
		return m_TemporarySinglePipeline.GetPipeline() == VK_NULL_HANDLE ? std::make_optional<std::reference_wrapper<const Pipeline>>(std::reference_wrapper<const Pipeline>(m_TemporarySinglePipeline)) : std::nullopt;
	}

	void PipelineManager::Destroy(VkDevice device)
	{
	}
}