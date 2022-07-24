#pragma once
#include "Utils/NonCopyable.h"
#include "backend/graphics/CommandBuffer.h"
#include "backend/graphics/GraphicsCaps.h"
#include <queue>
// 1. F*T pools, where F is the frame queue length and T is the number of threads that can concurrently record commands
// 2. target <10 submits and <100 cmb per frame

// current plan is to leave managing submits to something else

namespace imp 
{
	struct CommandPool
	{
		VkCommandPool pool;
		std::queue<CommandBuffer> readyPool;

		std::vector<CommandBuffer> AquireCommandBuffers(VkDevice device, uint32_t count);
	};

	class CommandBufferManager : NonCopyable
	{
	public:
		CommandBufferManager();
		void Initialize(VkDevice device, QueueFamilyIndices familyIndices, EngineSwapchainImageCount imageCount);

		void Submit(VkQueue submitQueue, std::vector<CommandBuffer> commandBuffers);
		void SignalFrameEnded();
		std::vector<CommandBuffer> AquireCommandBuffers(VkDevice device, uint32_t count);

		void Destroy(VkDevice device);
	private:
		uint32_t m_BufferingMode;
		uint32_t m_FrameClock;

		std::vector<CommandPool> m_GfxCommandPools;
	};
}