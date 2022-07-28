#include "Image.h"
#include <cassert>
#include <stdexcept>

imp::Image::Image()
    : m_Image(), m_ImageView(), m_ImageMemory()
{
}

imp::Image::Image(VkImage img, VkImageView imgView, VkDeviceMemory imgMem)
    : m_Image(img), m_ImageView(imgView), m_ImageMemory(imgMem)
{
}

void imp::Image::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkSampleCountFlagBits numSamples, MemoryProps memProps, VkMemoryPropertyFlags propFlags, VkDevice logicalDevice)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = useFlags;
    imageCreateInfo.samples = numSamples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &m_Image);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an image");

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(logicalDevice, m_Image, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = memProps.FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, propFlags);

    result = vkAllocateMemory(logicalDevice, &memoryAllocInfo, nullptr, &m_ImageMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate memory for image!");
    vkBindImageMemory(logicalDevice, m_Image, m_ImageMemory, 0);
}

void imp::Image::CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags, VkDevice logicalDevice)
{
    m_ImageView = Image::CreateImageView(m_Image, format, aspectFlags, logicalDevice);
}

VkImageView imp::Image::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice logicalDevice)
{
    assert(image);
    VkImageView imageView;

    VkImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.pNext = nullptr;
    viewCreateInfo.flags = 0;

    VkResult result = vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create an image view");

    return imageView;
}

void imp::Image::DestroyImage(VkDevice logicalDevice)
{
    vkDestroyImageView(logicalDevice, m_ImageView, nullptr);
    vkDestroyImage(logicalDevice, m_Image, nullptr);
    //if (m_ImageMemory)
        vkFreeMemory(logicalDevice, m_ImageMemory, nullptr);
   // else
     //   Debug::LogMsg("an Image was just destroyed that didn't have memory.. That's probably not right :)\0");
}
