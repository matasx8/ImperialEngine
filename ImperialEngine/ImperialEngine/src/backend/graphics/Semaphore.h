#pragma once
//#include "volk.h"
#include "backend/VulkanResource.h"

namespace imp
{
	struct Semaphore : public VulkanResource
	{
		Semaphore();
		Semaphore(VkSemaphore sem);
		Semaphore(VkSemaphore sem, uint64_t lastUsedInQueue);
		void Destroy(VkDevice device) override;

		VkSemaphore semaphore;
		uint64_t lastUsedInQueue;
	};
	
	struct SemaphoreFactory
	{
		Semaphore Create(VkDevice device);
		Semaphore operator()(VkDevice device);
	};
}