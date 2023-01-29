#include "VulkanBuffer.h"
#include "Utils/Pool.h"
#include <stdio.h>
#include <cassert>
#include <cstring>

namespace imp
{
	VulkanBuffer::VulkanBuffer()
		: m_Buffer(VK_NULL_HANDLE), m_Memory(VK_NULL_HANDLE), m_Size(), m_WriteOffset(), m_NumElements(), m_MemoryPtr(nullptr), m_TempOffset(), m_Fence()
	{
	}

	VulkanBuffer::VulkanBuffer(uint32_t size, VkBuffer buffer, VkDeviceMemory mem)
		: m_Buffer(buffer), m_Memory(mem), m_Size(size), m_WriteOffset(), m_NumElements(), m_MemoryPtr(nullptr), m_TempOffset(), m_Fence()
	{
	}

	uint32_t VulkanBuffer::GetSize() const
	{
		return m_Size;
	}

	VkBuffer VulkanBuffer::GetBuffer() const
	{
		return m_Buffer;
	}

	VkDeviceMemory VulkanBuffer::GetMemory() const
	{
		return m_Memory;
	}

	uint32_t VulkanBuffer::GetOffset() const
	{
		return m_WriteOffset;
	}

	void VulkanBuffer::resize(size_t size)
	{
		m_NumElements = size;
		// Should i also adjust write offset?
	}

	void VulkanBuffer::push_back(const void* data, size_t size)
	{
		// TODO compute-drawindirect: until I provide IGPUBuffer interface I'm going to assume each element is the same size
		assert(data);
		assert(size);
		const auto offset = m_NumElements * size;
		char* tempPtr = reinterpret_cast<char*>(m_MemoryPtr);
		tempPtr += offset;
		std::memcpy(tempPtr, data, size);
		m_NumElements++;
	}

	void VulkanBuffer::RegisterNewUpload(uint32_t size)
	{
		m_WriteOffset += size;
	}

	VkDescriptorBufferInfo VulkanBuffer::RegisterSubBuffer(size_t size)
	{
		// currently no misalignment protection - be safe
		VkDescriptorBufferInfo bi;
		bi.buffer = m_Buffer;
		bi.range = size;
		bi.offset = m_TempOffset;

		m_TempOffset += size;
		return bi;
	}

	uint32_t VulkanBuffer::FindNewSubBufferIndex(size_t size)
	{
		assert(size);
		assert(m_TempOffset % size == 0);
		return m_TempOffset / size;
	}

	bool VulkanBuffer::IsCurrentlyUsedByGPU(VkDevice device)
	{
		assert(m_Fence.fence != VK_NULL_HANDLE);
		const auto res = vkWaitForFences(device, 1, &m_Fence.fence, VK_TRUE, 0);
		return res == VK_TIMEOUT;
	}

	void VulkanBuffer::WaitUntilNotUsedByGPU(VkDevice device, PrimitivePool<Fence, FenceFactory>& m_FencePool)
	{
		if (m_Fence.fence != VK_NULL_HANDLE)
		{
			const auto res = vkWaitForFences(device, 1, &m_Fence.fence, VK_TRUE, 9999999999999999999); // TODO compute-drawIndirect: change to proper number
			assert(res == VK_SUCCESS);

			m_FencePool.Return(m_Fence);
			m_Fence.fence = VK_NULL_HANDLE;
		}
	}

	void VulkanBuffer::MapWholeBuffer(VkDevice device)
	{
		assert(IsMemoryMappedByHost() == false);
		const auto res = vkMapMemory(device, m_Memory, 0, m_Size, 0, &m_MemoryPtr);
		assert(res == VK_SUCCESS);
	}

	void VulkanBuffer::Destroy(VkDevice device)
	{
		if (IsMemoryMappedByHost())
			vkUnmapMemory(device, m_Memory);

		vkDestroyBuffer(device, m_Buffer, nullptr);
		vkFreeMemory(device, m_Memory, nullptr);
	}

	VulkanSubBuffer::VulkanSubBuffer()
		: m_Offset(0), m_Count(0)
	{
	}

	VulkanSubBuffer::VulkanSubBuffer(uint32_t offset, uint32_t count)
		: m_Offset(offset), m_Count(count)
	{
	}

	uint32_t VulkanSubBuffer::GetOffset() const
	{
		return m_Offset;
	}

	uint32_t VulkanSubBuffer::GetCount() const
	{
		return m_Count;
	}

	size_t VulkanSubBuffer::GetSize(size_t elementSize) const
	{
		return elementSize * m_Count;
	}

	void VulkanSubBuffer::Destroy(VkDevice device)
	{
		printf("TODO:");
	}
}