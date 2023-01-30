#include "Swapchain.h"
#include "backend/graphics/GraphicsCaps.h"
#include "Utils/Pool.h"
#include <stdexcept>
#include <cassert>
#include <backend/graphics/RenderPass/RenderPass.h>

imp::Swapchain::Swapchain(PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool)
    : m_Swapchain(), m_Format(), m_Extent(), m_PresentMode(), m_ImageCount(), m_SwapchainIndex(), m_FrameClock(), m_NeedsAcquiring(), m_SwapchainImages(), m_Semaphores(), m_SemaphorePool(semaphorePool)
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
    m_FrameClock = 0;
    m_NeedsAcquiring = true;

    PopulateNewSwapchainImages(device);
}

void imp::Swapchain::Present(VkQueue presentQ, const std::vector<VkSemaphore>& semaphores)
{
    VkResult res;
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = semaphores.size();
    presentInfo.pWaitSemaphores = semaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &m_SwapchainIndex;
    presentInfo.pResults = &res;

    auto result = vkQueuePresentKHR(presentQ, &presentInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present image");

    // I wonder should I use m_SwapchainIndex or m_FrameClock for stuff that relates to the number of swapchain images
    // YES use swapchain index
    m_FrameClock++;
    m_FrameClock %= m_ImageCount;
    m_NeedsAcquiring = true;
}

void imp::Swapchain::AcquireNextImage(VkDevice device, uint64_t currentFrame)
{
    // Get a random semaphore
    auto sem = m_SemaphorePool.Get(device, currentFrame).semaphore;
    // use that semaphore to be signaled when it's available
    const auto res = vkAcquireNextImageKHR(device, m_Swapchain, ~0ull, sem, VK_NULL_HANDLE, &m_SwapchainIndex);
    assert(res == VK_SUCCESS);
    // add that semaphore to the swapchain image that will be used
    m_SwapchainImages[m_SwapchainIndex].AddSemaphore(sem);
    m_NeedsAcquiring = false;
}

void imp::Swapchain::UpdateSwapchainImage(Surface& surf)
{
    m_SwapchainImages[m_SwapchainIndex] = surf;
}

imp::SurfaceDesc imp::Swapchain::GetSwapchainImageSurfaceDesc() const
{
    SurfaceDesc desc;
    desc.width = m_Extent.width;
    desc.height = m_Extent.height;
    desc.format = m_Format.format;
    desc.msaaCount = 1;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    desc.loadOp = kLoadOpDontCare;
    desc.storeOp = kStoreOpStore;
    desc.isColor = true;
    desc.isBackbuffer = true;
    return desc;
}

imp::Surface& imp::Swapchain::GetSwapchainImageSurface(VkDevice device, uint64_t currFrame)
{
    if (m_NeedsAcquiring)
        AcquireNextImage(device, currFrame);

    return m_SwapchainImages[m_SwapchainIndex];
}

uint32_t imp::Swapchain::GetSwapchainImageCount() const
{
    return m_ImageCount;
}

uint32_t imp::Swapchain::GetFrameClock() const
{
    return m_FrameClock;
}

void imp::Swapchain::Destroy(VkDevice device)
{
    for (auto& image : m_SwapchainImages)
    {       
        vkDestroyImageView(device, image.GetImage().GetImageView(), nullptr);
        vkDestroySemaphore(device, image.GetSemaphore(), nullptr);
    }
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
        SurfaceDesc desc = GetSwapchainImageSurfaceDesc();

        const uint64_t frameLastUsed = ~0ull; // should never delete swapchain images
        m_SwapchainImages[i] = Surface(img, desc, frameLastUsed);
        i++;
    }
}
