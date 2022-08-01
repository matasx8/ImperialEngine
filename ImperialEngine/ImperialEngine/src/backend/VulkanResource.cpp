#include "VulkanResource.h"

imp::VulkanResource::VulkanResource()
	: m_FrameLastUsed(0)
{
}

void imp::VulkanResource::UpdateLastUsed(uint64_t currentFrame)
{
	m_FrameLastUsed = currentFrame;
}

uint64_t imp::VulkanResource::GetLastUsed() const
{
	return m_FrameLastUsed;
}

void imp::VulkanResource::Destroy(VkDevice device)
{
}
