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

		void CreatePipeline(const RenderPassBase& rp);
		const Pipeline& GetOrCreatePipeline(const RenderPassBase& rp);
		auto GetPipeline() const;

		void Destroy(VkDevice device);

	private:

		// TODO: have an O(1) get container of pipelines
		Pipeline m_TemporarySinglePipeline;
	};
}