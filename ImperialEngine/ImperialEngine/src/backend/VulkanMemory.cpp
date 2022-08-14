#include "VulkanMemory.h"
#include "backend/graphics/GraphicsCaps.h"

namespace imp
{

	VulkanMemory::VulkanMemory()
	{
	}

	void VulkanMemory::Initialize()
	{
	}

	VulkanBuffer VulkanMemory::GetBuffer(VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags buffMemPropFlags, const MemoryProps& memoryProps)
	{
		VkBuffer buffer;

		VkBufferCreateInfo bufferInfo;
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // means only this queue family can access. Means will have to change if I introduce seperate transfer queue
		bufferInfo.queueFamilyIndexCount = 0; // ignored when VK_SHARING_MODE_EXCLUSIVE (TODO: maybe don't have to init this memory?)
		bufferInfo.pQueueFamilyIndices = nullptr;
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
