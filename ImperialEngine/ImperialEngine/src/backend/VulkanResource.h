#pragma once
#include <vulkan_core.h>

namespace imp
{
	class VulkanResource
	{
	public:
		VulkanResource();

		void UpdateLastUsed(uint64_t currentFrame);
		uint64_t GetLastUsed() const;
		bool HasSemaphore() const;
		VkSemaphore GetSemaphore() const;
		VkSemaphore StealSemaphore();
		void GiveSemaphore(VkSemaphore& sem);

		virtual void Destroy(VkDevice device); // why didn't I make this pure?

	protected:

		uint64_t m_FrameLastUsed;
		VkSemaphore m_Semaphore;
	};
}