#include "VulkanMemory.h"
#include "backend/graphics/GraphicsCaps.h"

namespace imp
{

	VulkanMemory::VulkanMemory()
	{
	}

	VulkanBuffer VulkanMemory::GetBuffer(VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags buffMemPropFlags, const MemoryProps& memoryProps)
	{
		VkBuffer buffer;

		VkBufferCreateInfo bufferInfo;
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.queueFamilyIndexCount = 0;
		bufferInfo.pQueueFamilyIndices = nullptr;
		bufferInfo.pNext = nullptr;
		bufferInfo.flags = 0;

		auto res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
		
		if (res != VK_SUCCESS && bufferUsageFlags == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			printf("[Shader Memory] Device did not have enough DEVICE_LOCAL + HOST_VISIBLE memory. Falling back to only HOST_VISIBLE\n");
			bufferInfo.usage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
		}
		assert(res == VK_SUCCESS);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
		const auto mem = AllocateMemory(device, memRequirements.size, memRequirements, buffMemPropFlags, memoryProps);
		res = vkBindBufferMemory(device, buffer, mem, 0);
		assert(res == VK_SUCCESS);

		VkSemaphoreTypeCreateInfo timelineCreateInfo;
		timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		timelineCreateInfo.pNext = NULL;
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = 0ull;

		VkSemaphoreCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = &timelineCreateInfo;
		createInfo.flags = 0;

		VkSemaphore timelineSemaphore;
		vkCreateSemaphore(device, &createInfo, NULL, &timelineSemaphore);

		VulkanBuffer buff(bufferSize, buffer, mem, timelineSemaphore);
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
}
