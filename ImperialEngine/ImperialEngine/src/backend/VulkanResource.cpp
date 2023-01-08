#include "VulkanResource.h"
#include <cassert>

imp::VulkanResource::VulkanResource()
	: m_FrameLastUsed(0), m_Semaphore(VK_NULL_HANDLE)
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

bool imp::VulkanResource::HasSemaphore() const
{
	return m_Semaphore != VK_NULL_HANDLE;
}

VkSemaphore imp::VulkanResource::GetSemaphore() const
{
	return m_Semaphore;
}

VkSemaphore imp::VulkanResource::StealSemaphore()
{
	assert(HasSemaphore()); // if we're trying to steal a null semaphore then there's something wrong probably
	const auto tempCopy = m_Semaphore;
	m_Semaphore = VK_NULL_HANDLE;
	return tempCopy;
}

void imp::VulkanResource::GiveSemaphore(VkSemaphore& sem)
{
	//assert(!HasSemaphore());
	m_Semaphore = sem;
}

void imp::VulkanResource::Destroy(VkDevice device)
{
}
