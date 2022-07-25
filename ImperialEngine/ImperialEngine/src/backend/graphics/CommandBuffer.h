#pragma once
#include <vulkan.h>

namespace imp
{
	class CommandBuffer
	{
	public:
		CommandBuffer();
		CommandBuffer(VkCommandBuffer cb);

		VkCommandBuffer cmb;
	private:

	};
}