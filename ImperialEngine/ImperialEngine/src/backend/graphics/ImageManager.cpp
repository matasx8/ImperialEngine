#include "ImageManager.h"
#include "backend/VulkanMemory.h"
#include "backend/VariousTypeDefinitions.h"
#include "backend/graphics/GraphicsCaps.h"
#include "Utils/GfxUtilities.h"

namespace imp
{
    ImageManager::ImageManager(VulkanMemory& memory)
        : m_Memory(memory)
    {

    }

    static void SetImageName(VkDevice device, VkImage image, const char* name)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t)(image);
        nameInfo.pObjectName = name;

#if _DEBUG
        vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
#endif
    }

    VulkanImage ImageManager::GetImage(VkDevice device, const ImageCreationRequest& req, const MemoryProps& memProps)
    {
        VkImageCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.extent.width = req.width;
        ci.extent.height = req.height;
        ci.extent.depth = 1;
        ci.mipLevels = 1;
        ci.arrayLayers = 1;
        ci.format = VK_FORMAT_R8G8B8A8_SRGB;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkImage image;
        const auto result = vkCreateImage(device, &ci, nullptr, &image);
        assert(result == VK_SUCCESS);

        SetImageName(device, image, req.name.c_str());

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, image, &memReqs);
        const auto memory = m_Memory.AllocateMemory(device, memReqs.size, memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memProps);

        const auto result2 = vkBindImageMemory(device, image, memory, 0);
        assert(result2 == VK_SUCCESS);

        VulkanImage finalImage { req.name, image, memory, memReqs.size, ci.imageType, ci.format, ci.extent, ci.initialLayout, utils::CreateTimelineSemaphore(device) };
        return finalImage;
    }
}
