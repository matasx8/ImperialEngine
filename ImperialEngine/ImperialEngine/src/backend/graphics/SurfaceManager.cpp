#include "SurfaceManager.h"
#include "backend/graphics/RenderPass/RenderPass.h"
#include "backend/graphics/Swapchain.h"
#include <stdexcept>

imp::SurfaceManager::SurfaceManager()
    : m_SurfacePool(), m_MemoryProps()
{
}

void imp::SurfaceManager::Initialize(VkDevice device, MemoryProps deviceMemoryProps)
{
    m_MemoryProps = deviceMemoryProps;
}

//imp::Framebuffer imp::SurfaceManager::GetFramebuffer(const RenderPass& rp, VkDevice device, const std::vector<Surface>& surfaces)
//{
//    auto fb = CreateFramebuffer(rp, surfaces, device);
//    return fb;
//}

imp::Surface imp::SurfaceManager::GetSurface(const SurfaceDesc& desc, VkDevice device)
{
	const auto surf = m_SurfacePool.extract(desc);
	if (surf.empty())
		return CreateSurface(desc, device);
	return surf.mapped();
}

void imp::SurfaceManager::ReturnSurfaces(std::vector<Surface>& surfaces, Swapchain& swapchainWorkaround)
{
    for (auto& surf : surfaces)
    {
        auto desc = surf.GetDesc();
        if (!desc.isBackbuffer) // leave backbuffers to swapchain
            m_SurfacePool.insert(std::make_pair(desc, surf));
        else
            swapchainWorkaround.UpdateSwapchainImage(surf);
    }
}

void imp::SurfaceManager::SignalFrameEnded()
{
    // Surfaces may have semaphores but have never been used anymore so they might end up with
    // fake dependencies that mess up the next frame. Here we can remove those
    // TODO SYNC: recycle
    for (auto& surf : m_SurfacePool)
        surf.second.RemoveSemaphore();
}

void imp::SurfaceManager::CombForUnusedSurfaces()
{
	// unordered map is actually okay, can save the .first values that are old and need to be extracted
}

void imp::SurfaceManager::Destroy(VkDevice device)
{
    for (auto& surf : m_SurfacePool)
        surf.second.Destroy(device);
}

imp::Surface imp::SurfaceManager::CreateSurface(const SurfaceDesc& desc, VkDevice device)
{
    Image img;
    VkImageUsageFlagBits attachmentBit = desc.isColor ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageAspectFlagBits aspectBit = desc.isColor ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;

    if (desc.isColor)
    {
        img.CreateImage(desc.width, desc.height, desc.format,
            VK_IMAGE_TILING_OPTIMAL, attachmentBit | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            static_cast<VkSampleCountFlagBits>(desc.msaaCount), m_MemoryProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device);
    }
    else
        img.CreateImage(desc.width, desc.height, desc.format,
            VK_IMAGE_TILING_OPTIMAL, attachmentBit,
            static_cast<VkSampleCountFlagBits>(desc.msaaCount), m_MemoryProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device);

    // This is not good, now only creating one specific view
    // when time comes improve this
    img.CreateImageView(desc.format, aspectBit, device);

    Surface surf(img, desc, 0ull);
    return surf;
}

static std::vector<VkImageView> GetImageViews(const std::vector<imp::Surface>& surfaces)
{
    std::vector<VkImageView> views;
    for (const auto& surf : surfaces)
        views.emplace_back(surf.GetImage().GetImageView());
    return views;
}

imp::Framebuffer imp::SurfaceManager::CreateFramebuffer(const RenderPass& rp, const std::vector<Surface>& surfaces, VkDevice device)
{
    assert(surfaces.size());
    const auto desc = rp.GetRenderPassDesc();
    const auto views = GetImageViews(surfaces);
    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = rp.GetVkRenderPass();
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(surfaces.size());
    framebufferCreateInfo.pAttachments = views.data();
    framebufferCreateInfo.width = surfaces[0].GetDesc().width;
    framebufferCreateInfo.height = surfaces[0].GetDesc().height;
    framebufferCreateInfo.layers = 1;

    VkFramebuffer fb;
    VkResult result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &fb);
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Framebuffer");
    return Framebuffer(fb);
}
