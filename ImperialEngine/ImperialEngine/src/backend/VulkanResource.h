#pragma once
#include <vulkan_core.h>

namespace imp
{
	class CommandBuffer;

	struct CountedResource
	{
		CountedResource() : m_FrameLastUsed() {};
		virtual ~CountedResource() {};

		void UpdateLastUsed(uint64_t currentFrame)
		{
			m_FrameLastUsed = currentFrame;
		}

		uint64_t GetLastUsed() const
		{
			return m_FrameLastUsed;
		};

	protected:
		uint64_t m_FrameLastUsed;
	};

	struct TimelineSemaphore
	{
		VkSemaphore semaphore;
		uint64_t lastUsedInQueue;
	};

	class VulkanResource : public CountedResource
	{
	public:
		VulkanResource();

		bool HasSemaphore() const;
		VkSemaphore GetSemaphore() const;
		VkSemaphore StealSemaphore();
		void GiveSemaphore(VkSemaphore& sem);

		// timeline
		void MarkUsedInQueue();
		TimelineSemaphore GetTimeline() const;
		void MakeSureNotUsedOnGPU(VkDevice device);

		virtual void Destroy(VkDevice device);
		virtual ~VulkanResource() {};

	protected:
		VkSemaphore m_Semaphore;
		VkSemaphore m_TimelineSemaphore;

		// Incremented by Queue that used this resource
		// Next Queue operation to use this should wait on this value
		uint64_t m_UsedInTimeline;
	};
}