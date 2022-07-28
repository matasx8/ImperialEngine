#pragma once
#include <vulkan.h>

namespace imp
{
	class CommandBuffer
	{
	public:
		CommandBuffer();
		CommandBuffer(VkCommandBuffer cb);

		void Begin();
		void End();

		VkCommandBuffer cmb;
	private:

	};
}