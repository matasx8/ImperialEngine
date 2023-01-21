#include "VulkanResource.h"
#include <cassert>

imp::VulkanResource::VulkanResource()
	: CountedResource(), m_Semaphore(VK_NULL_HANDLE)
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

void imp::VulkanResource::Destroy(VkDevice device)
{
}
