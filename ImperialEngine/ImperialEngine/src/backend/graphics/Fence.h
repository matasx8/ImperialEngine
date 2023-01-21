#pragma once
#include "backend/VulkanResource.h"

namespace imp
{
	struct Fence : public VulkanResource
	{
		Fence(VkFence sem);
		void Destroy(VkDevice device) override;

		VkFence fence;
	};

	struct FenceFactory
	{
		Fence Create(VkDevice device);
		Fence operator()(VkDevice device);
	};
}