#pragma once
#include "backend/VulkanResource.h"

namespace imp
{
	struct Semaphore : public VulkanResource
	{
		Semaphore();
		Semaphore(VkSemaphore sem);
		void Destroy(VkDevice device) override;

		VkSemaphore semaphore;
	};
	
	struct SemaphoreFactory
	{
		Semaphore Create(VkDevice device);
		Semaphore operator()(VkDevice device);
	};
}