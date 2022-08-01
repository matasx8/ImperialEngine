#pragma once
#include <vulkan.h>
#include <backend/graphics/GraphicsCaps.h>

namespace imp
{
	class Image
	{
	public:
		Image();
		Image(VkImage img, VkImageView imgView, VkDeviceMemory imgMem);
		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkSampleCountFlagBits numSamples, MemoryProps memProps, VkMemoryPropertyFlags propFlags, VkDevice logicalDevice);
		void CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags, VkDevice logicalDevice);
		static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice logicalDevice);

		VkImage GetImage() const { return m_Image; }
		VkImageView GetImageView() const { return m_ImageView; }

		void Destroy(VkDevice logicalDevice);

	private:
		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_ImageMemory;
	};
}
