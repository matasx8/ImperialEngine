#pragma once
#include "VulkanResource.h"

namespace imp
{
	class VulkanBuffer : public VulkanResource
	{
	public:
		VulkanBuffer();
		VulkanBuffer(uint32_t size, VkBuffer buffer, VkDeviceMemory mem);

		uint32_t GetSize() const;
		VkBuffer GetBuffer() const;
		VkDeviceMemory GetMemory() const;

		void Destroy(VkDevice device) override;

	private:

		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;
		uint32_t m_Size; // in bytes
	};

	class VulkanSubBuffer : public VulkanResource
	{
	public:
		VulkanSubBuffer();
		VulkanSubBuffer(uint32_t offset, uint32_t count);

		uint32_t GetOffset() const;
		uint32_t GetCount() const;
		size_t GetSize(size_t elementSize) const;

		void Destroy(VkDevice device) override;

	private:
		// make this into struct so we can safely add more members
		uint32_t m_Offset;
		uint32_t m_Count;
	};
}