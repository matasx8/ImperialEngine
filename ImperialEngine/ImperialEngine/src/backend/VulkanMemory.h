#pragma once
#include "Utils/NonCopyable.h"
#include "backend/VulkanBuffer.h"

namespace imp
{
	struct MemoryProps;

	class VulkanMemory : NonCopyable
	{
	public:
		VulkanMemory();
		void Initialize(VkDevice device);

		VulkanBuffer GetBuffer(VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags buffMemPropFlags, const MemoryProps& memoryProps);
		VkDeviceMemory AllocateMemory(VkDevice device, VkDeviceSize bufferSize, VkMemoryRequirements memReqs, VkMemoryPropertyFlags buffMemPropFlags, const MemoryProps& memoryProps);

		void Destroy();

	private:


	};
}