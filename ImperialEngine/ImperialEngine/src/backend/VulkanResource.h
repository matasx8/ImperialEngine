#pragma once
#include <vulkan.h>

namespace imp
{
	class VulkanResource
	{
	public:
		VulkanResource();

		void UpdateLastUsed(uint64_t currentFrame);
		uint64_t GetLastUsed() const;

		virtual void Destroy(VkDevice device);

	protected:

		uint64_t m_FrameLastUsed;

	};
}