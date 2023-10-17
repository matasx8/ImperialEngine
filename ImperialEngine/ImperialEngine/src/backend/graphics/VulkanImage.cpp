#include "VulkanImage.h"

namespace imp
{
	VulkanImage::VulkanImage(const std::string& name, VkImage image, VkDeviceMemory memory, uint64_t memorySize, VkImageType imageType, VkFormat format, VkExtent3D extent, VkImageLayout layout, VkSemaphore timelineSemaphore)
		: m_Name(name)
		, m_Image(image)
		, m_Memory(memory)
		, m_MemorySize(memorySize)
		, m_ImageType(imageType)
		, m_Format(format)
		, m_Extent(extent)
		, m_Layout(layout)
	{
		m_TimelineSemaphore = timelineSemaphore;
	}

	VkImage VulkanImage::GetImage() const
	{
		return m_Image;
	}

	VkImageLayout VulkanImage::GetLayout() const
	{
		return m_Layout;
	}

	uint64_t VulkanImage::GetSize() const
	{
		return m_MemorySize;
	}

	VkExtent3D VulkanImage::GetExtent() const
	{
		return m_Extent;
	}

	VkFormat VulkanImage::GetFormat() const
	{
		return m_Format;
	}

	void VulkanImage::SetLayout(VkImageLayout newLayout)
	{
		m_Layout = newLayout;
	}

}
