#pragma once
#include "VulkanResource.h"

namespace imp
{
	class VulkanBuffer : public VulkanResource
	{
	public:
		VulkanBuffer();

		uint32_t GetSize() const;
		VkBuffer GetBuffer() const;
		VkDeviceMemory GetMemory() const;

		void Destroy(VkDevice device) override;

	private:

		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;
		uint32_t m_Size; // in bytes
	};
}