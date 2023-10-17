#pragma once
#include "backend/VulkanResource.h"
#include <string>

namespace imp
{
	class VulkanImage : public VulkanResource
	{
	public:
		VulkanImage(const std::string& name, VkImage image, VkDeviceMemory memory, uint64_t memorySize, VkImageType imageType, VkFormat format, VkExtent3D extent, VkImageLayout layout, VkSemaphore timelineSemaphore);

		VkImage GetImage() const;
		VkImageLayout GetLayout() const;
		uint64_t GetSize() const;
		VkExtent3D GetExtent() const;
		VkFormat GetFormat() const;
 		void SetLayout(VkImageLayout newLayout);

	private:
		std::string m_Name;

		VkImage m_Image;
		VkDeviceMemory m_Memory;

		uint64_t m_MemorySize;
        VkImageType m_ImageType;
        VkFormat m_Format;
        VkExtent3D m_Extent;
        VkImageLayout m_Layout;
	};
}