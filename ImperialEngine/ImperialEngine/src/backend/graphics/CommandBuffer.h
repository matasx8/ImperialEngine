#pragma once
#include <vulkan_core.h>

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

		VkCommandBuffer cmb;
	private:
		uint32_t m_CurrentStage; // CommandBufferStage
	};
}