#pragma once
#include "volk.h"

namespace imp
{
	enum CommandBufferStage : uint32_t
	{
		kCBStageNew,
		kCBStageActive,
		kCBStageDone
	};

	class CommandBuffer
	{
	public:
		CommandBuffer();
		CommandBuffer(VkCommandBuffer cb);

		void Begin();
		CommandBufferStage GetCurrentStage() const;
		void End();
		void ResetStageToNew();
		void InitializeEmpty();

		VkCommandBuffer cmb;
	private:
		uint32_t m_CurrentStage; // CommandBufferStage
	};
}