#include "Semaphore.h"
#include <cassert>

namespace imp
{
	Semaphore SemaphoreFactory::Create(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore sem;
		const auto res = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sem);
		assert(res == VK_SUCCESS);
		return Semaphore(sem);
	}

	Semaphore SemaphoreFactory::operator()(VkDevice device)
	{
		return Create(device);
	}

	Semaphore::Semaphore() : semaphore(VK_NULL_HANDLE), lastUsedInQueue() {};
	Semaphore::Semaphore(VkSemaphore sem) : semaphore(sem), lastUsedInQueue() {};
	Semaphore::Semaphore(VkSemaphore sem, uint64_t lastUsedInQueue) : semaphore(sem), lastUsedInQueue(lastUsedInQueue) {};

	void Semaphore::Destroy(VkDevice device)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
#if _DEBUG
		semaphore = 0;
#endif
	}

}