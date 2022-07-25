#include "Swapchain.h"
#include "backend/graphics/GraphicsCaps.h"
#include <stdexcept>

imp::Swapchain::Swapchain()
{
}

void imp::Swapchain::Create(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface, const EngineGraphicsSettings& gfxSettings, const PhysicalDeviceSurfaceCaps& surfaceCaps, VkExtent2D extent)
{
	VkSurfaceFormatKHR surfaceFormat = GraphicsCaps::ChooseBestSurfaceFormat(surfaceCaps.formats);
	VkPresentModeKHR presentMode = GraphicsCaps::ChooseBestPresentationMode(surfaceCaps.presentationModes, gfxSettings);
	VkExtent2D chosenExtent = GraphicsCaps::ChooseBestExtent(surfaceCaps.surfaceCapabilities, extent);
	uint32_t swapchainImageCount = GraphicsCaps::ChooseSwapchainImageCount(surfaceCaps.surfaceCapabilities, gfxSettings.swapchainImageCount);

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = windowSurface;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.imageExtent = chosenExtent;
    swapChainCreateInfo.minImageCount = swapchainImageCount;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapChainCreateInfo.preTransform = surfaceCaps.surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.clipped = VK_TRUE; // false if screenshots or recording enabled?
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    const auto indices = GraphicsCaps::GetQueueFamilies(physicalDevice, windowSurface);
    //if graphics and presentation families are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        uint32_t queueFamilyIndices[] = {
            (uint32_t)indices.graphicsFamily,
            (uint32_t)indices.presentationFamily
        };

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 2; //number of queues to share between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices; //array of queues to share between
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkResult result = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &m_Swapchain);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a swapchain");

    m_Extent = chosenExtent;
    m_Format = surfaceFormat;
    m_ImageCount = swapchainImageCount;
    m_PresentMode = presentMode;

    PopulateNewSwapchainImages(device);
}

imp::SurfaceDesc imp::Swapchain::GetSwapchainImageSurfaceDesc()
{
    SurfaceDesc desc;
    desc.width = m_Extent.width;
    desc.height = m_Extent.height;
    desc.format = m_Format.format;
    desc.msaaCount = 1;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    desc.isColor = true;
    return desc;
}

void imp::Swapchain::Destroy(VkDevice device)
{
    for (auto& image : m_SwapchainImages)
        vkDestroyImageView(device, image.GetImage().GetImageView(), nullptr);
    vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
}

void imp::Swapchain::PopulateNewSwapchainImages(VkDevice device)
{
    std::array<VkImage, kEngineSwapchainExclusiveMax - 1> images;
    uint32_t count;
    VkResult result = vkGetSwapchainImagesKHR(device, m_Swapchain, &count, images.data());
    if(result != VK_SUCCESS && count != m_ImageCount)
        throw std::runtime_error("Failed to get Swapchain Images!");

    int i = 0;
    for (VkImage image : images)
    {
        VkImageView imageView = Image::CreateImageView(image, m_Format.format, VK_IMAGE_ASPECT_COLOR_BIT, device);
        Image img(image, imageView, 0);
        const uint32_t numSamples = 1;
        SurfaceDesc desc = { m_Extent.width, m_Extent.height, m_Format.format, numSamples };
        const uint64_t frameLastUsed = ~0ull;
        m_SwapchainImages[i++] = Surface(img, desc, frameLastUsed);
    }
}
