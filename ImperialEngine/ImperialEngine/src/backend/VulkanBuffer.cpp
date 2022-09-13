#include "VulkanBuffer.h"
#include <stdio.h>

imp::VulkanBuffer::VulkanBuffer()
	: m_Buffer(VK_NULL_HANDLE), m_Memory(VK_NULL_HANDLE), m_Size(0)
{
}

imp::VulkanBuffer::VulkanBuffer(uint32_t size, VkBuffer buffer, VkDeviceMemory mem)
	: m_Buffer(buffer), m_Memory(mem), m_Size(size)
{
}

uint32_t imp::VulkanBuffer::GetSize() const
{
	return m_Size;
}

VkBuffer imp::VulkanBuffer::GetBuffer() const
{
	return m_Buffer;
}

VkDeviceMemory imp::VulkanBuffer::GetMemory() const
{
	return m_Memory;
}

void imp::VulkanBuffer::Destroy(VkDevice device)
{
	vkDestroyBuffer(device, m_Buffer, nullptr);
	vkFreeMemory(device, m_Memory, nullptr);
}

imp::VulkanSubBuffer::VulkanSubBuffer()
	: m_Offset(0), m_Count(0)
{
}

imp::VulkanSubBuffer::VulkanSubBuffer(uint32_t offset, uint32_t count)
	: m_Offset(offset), m_Count(count)
{
}

uint32_t imp::VulkanSubBuffer::GetOffset() const
{
	return m_Offset;
}

uint32_t imp::VulkanSubBuffer::GetCount() const
{
	return m_Count;
}

size_t imp::VulkanSubBuffer::GetSize(size_t elementSize) const
{
	return elementSize * m_Count;
}

void imp::VulkanSubBuffer::Destroy(VkDevice device)
{
	printf("TODO:");
}
