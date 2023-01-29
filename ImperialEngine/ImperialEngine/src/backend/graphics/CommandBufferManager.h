#pragma once
#include "Utils/NonCopyable.h"
#include "backend/graphics/CommandBuffer.h"
#include "backend/graphics/GraphicsCaps.h"
#include "backend/graphics/Semaphore.h"
#include "backend/graphics/Fence.h"
#include <queue>
// 1. F*T pools, where F is the frame queue length and T is the number of threads that can concurrently record commands
// 2. target <10 submits and <100 cmb per frame

// current plan is to leave managing submits to something else

namespace imp 
{
	template<typename T,
		typename FactoryFunction>
	class PrimitivePool;

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
		void Reset(VkDevice device, PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool);
	};

	struct SubmitSynchPrimitives
	{
		Semaphore semaphore;
		Fence fence;
	};

	class CommandBufferManager : NonCopyable
	{
	public:
		CommandBufferManager(PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool, PrimitivePool<Fence, FenceFactory>& fencePool);
		void Initialize(VkDevice device, QueueFamilyIndices familyIndices, EngineSwapchainImageCount imageCount);

		// Submit command buffer to internal command buffer queue. Will keep them until SubmitToQueue is called.
		void SubmitInternal(CommandBuffer& cb);
		void SubmitInternal(CommandBuffer& cb, const std::vector<Semaphore>& semaphores);

		// Submit accumulated command buffers to VkQueue
		SubmitSynchPrimitives SubmitToQueue(VkQueue submitQueue, VkDevice device, SubmitType submitType, uint64_t currFrame);
		void SignalFrameEnded();
		std::vector<CommandBuffer> AquireCommandBuffers(VkDevice device, uint32_t count);
		CommandBuffer AquireCommandBuffer(VkDevice device);
		std::vector<VkSemaphore>& GetCommandExecSemaphores();
		const Fence& GetCurrentFence() const;

		void Destroy(VkDevice device);
	private:

		uint32_t m_BufferingMode;
		uint32_t m_FrameClock;
		bool m_IsNewFrame;

		std::vector<CommandPool> m_GfxCommandPools;
		std::vector<CommandBuffer> m_CommandsBuffersToSubmit;
		std::vector<Semaphore> m_SemaphoresToWaitOnSubmit;
		Fence m_CurrentFence;

		PrimitivePool<Semaphore, SemaphoreFactory>& m_SemaphorePool;
		PrimitivePool<Fence, FenceFactory>& m_FencePool;
	};
}