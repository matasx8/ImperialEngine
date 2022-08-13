#include "VulkanBuffer.h"

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
