#pragma once
#include <vulkan.h>

namespace imp
{
	class Image
	{
	public:
		Image();
		Image(VkImage img, VkImageView imgView, VkDeviceMemory imgMem);
		void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkSampleCountFlagBits numSamples, VkMemoryPropertyFlags propFlags, VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
		void createImageView(VkFormat format, VkImageAspectFlags aspectFlags, VkDevice logicalDevice);
		static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice logicalDevice);

		VkImage getImage() const { return m_Image; }
		VkImageView getImageView() const { return m_ImageView; }

		void destroyImage(VkDevice logicalDevice);

	private:
		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_ImageMemory;
	};
}
