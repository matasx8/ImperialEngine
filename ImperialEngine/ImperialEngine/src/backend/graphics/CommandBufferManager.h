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

	enum SubmitType
	{
		// After submit will return synch primitives for caller to take care of
		kSubmitDontCare,
		// After submit will keep synch primitives and wait on before presenting
		kSubmitSynchForPresent
	};
	struct CommandPool
	{
		VkCommandPool pool;
		std::queue<CommandBuffer> readyPool;
		std::vector<CommandBuffer> donePool;
		std::vector<VkSemaphore> semaphores;
		std::vector<VkFence> fences;

		std::vector<CommandBuffer> AquireCommandBuffers(VkDevice device, uint32_t count);
		void ReturnCommandBuffers(std::vector<CommandBuffer>& donePool, VkSemaphore semaphore, VkFence fence);
		void Reset(VkDevice device);
	};

	struct SubmitSynchPrimitives
	{
		VkSemaphore semaphore;
		VkFence fence;
	};

	class CommandBufferManager : NonCopyable
	{
	public:
		CommandBufferManager();
		void Initialize(VkDevice device, QueueFamilyIndices familyIndices, EngineSwapchainImageCount imageCount);

		SubmitSynchPrimitives Submit(VkQueue submitQueue, VkDevice device, std::vector<CommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSemaphores, SubmitType submitType);
		void SignalFrameEnded();
		std::vector<CommandBuffer> AquireCommandBuffers(VkDevice device, uint32_t count);
		CommandBuffer AquireCommandBuffer(VkDevice device);
		std::vector<VkSemaphore>& GetCommandExecSemaphores();

		void Destroy(VkDevice device);
	private:

		VkSemaphore GetSemaphore(VkDevice device);
		VkFence GetFence(VkDevice device);

		uint32_t m_BufferingMode;
		uint32_t m_FrameClock;
		bool m_IsNewFrame;

		std::vector<CommandPool> m_GfxCommandPools;
	};
}