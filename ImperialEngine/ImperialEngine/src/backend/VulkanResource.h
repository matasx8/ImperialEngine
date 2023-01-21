#pragma once
#include <vulkan_core.h>

namespace imp
{
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
	class VulkanResource : public CountedResource
	{
	public:
		VulkanResource();

		bool HasSemaphore() const;
		VkSemaphore GetSemaphore() const;
		VkSemaphore StealSemaphore();
		void GiveSemaphore(VkSemaphore& sem);

		virtual void Destroy(VkDevice device);
		virtual ~VulkanResource() {};

	protected:
		VkSemaphore m_Semaphore;
	};
}