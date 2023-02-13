#include "VulkanResource.h"
#include "backend/graphics/CommandBuffer.h"
#include "graphics/Semaphore.h"
#include <cassert>

imp::VulkanResource::VulkanResource()
	: CountedResource(), m_Semaphore(VK_NULL_HANDLE), m_TimelineSemaphore(VK_NULL_HANDLE)
{
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
	// TODO BUFFER UPLOAD:
	// Currently this is not very correct. I'm basically appending data to a vulkan buffer and assigning a semaphore for the whole buffer
	// When I should give the semaphore only for the sub-buffers or something like that. Because other areas won't be affected.
	//assert(!HasSemaphore());
	m_Semaphore = sem;
}

void imp::VulkanResource::MarkUsedInQueue()
{
	m_UsedInTimeline++;
}

imp::TimelineSemaphore imp::VulkanResource::GetTimeline() const
{
	return TimelineSemaphore(m_TimelineSemaphore, m_UsedInTimeline);
}

void imp::VulkanResource::MakeSureNotUsedOnGPU(VkDevice device)
{
	assert(m_TimelineSemaphore != VK_NULL_HANDLE);
	VkSemaphoreWaitInfo wi = {};
	wi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wi.semaphoreCount = 1;
	wi.pSemaphores = &m_TimelineSemaphore;
	wi.pValues = &m_UsedInTimeline;

	// if m_UsedInTimeline is less than the actual value in the semaphore then it should be no-op.
	vkWaitSemaphores(device, &wi, UINT64_MAX);
}

void imp::VulkanResource::Destroy(VkDevice device)
{
}
