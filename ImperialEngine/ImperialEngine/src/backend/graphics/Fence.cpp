#include "Fence.h"
#include <cassert>

namespace imp
{
	Fence FenceFactory::Create(VkDevice device)
	{
		VkFenceCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence sem;
		const auto res = vkCreateFence(device, &semaphoreCreateInfo, nullptr, &sem);
		assert(res == VK_SUCCESS);
		return Fence(sem);
	}

	Fence FenceFactory::operator()(VkDevice device)
	{
		return Create(device);
	}

	Fence::Fence(VkFence fence) : fence(fence) {};

	void Fence::Destroy(VkDevice device)
	{
		vkDestroyFence(device, fence, nullptr);
#if _DEBUG
		fence = 0;
#endif
	}

}