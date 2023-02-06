#include "VulkanMemory.h"
#include "backend/graphics/GraphicsCaps.h"

namespace imp
{

	VulkanMemory::VulkanMemory()
	{
	}

	void VulkanMemory::Initialize(VkDevice device)
	{

	}

	VulkanBuffer VulkanMemory::GetBuffer(VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags buffMemPropFlags, const MemoryProps& memoryProps)
	{
		VkBuffer buffer;

		VkBufferCreateInfo bufferInfo;
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.queueFamilyIndexCount = 2;
		uint32_t indices[2] = { 0, 1 };
		bufferInfo.pQueueFamilyIndices = indices;
		bufferInfo.pNext = nullptr;
		bufferInfo.flags = 0;

		auto res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
		assert(res == VK_SUCCESS);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
		const auto mem = AllocateMemory(device, memRequirements.size, memRequirements, buffMemPropFlags, memoryProps);
		res = vkBindBufferMemory(device, buffer, mem, 0);
		assert(res == VK_SUCCESS);

		VulkanBuffer buff(bufferSize, buffer, mem);
		return buff;
	}

	VkDeviceMemory VulkanMemory::AllocateMemory(VkDevice device, VkDeviceSize bufferSize, VkMemoryRequirements memReqs, VkMemoryPropertyFlags buffMemPropFlags, const MemoryProps& memoryProps)
	{
		VkDeviceMemory mem;
		assert(memReqs.size);

		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memReqs.size;
		memoryAllocInfo.memoryTypeIndex = memoryProps.FindMemoryTypeIndex(memReqs.memoryTypeBits, buffMemPropFlags);
		const auto res = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &mem);
		assert(res == VK_SUCCESS);

		return mem;
	}

	void VulkanMemory::Destroy()
	{
	}
}
