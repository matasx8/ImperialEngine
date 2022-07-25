#include "SurfaceManager.h"
#include "backend/graphics/RenderPassBase.h"

imp::SurfaceManager::SurfaceManager()
{
}

void imp::SurfaceManager::Initialize(VkDevice device)
{
}

imp::Framebuffer imp::SurfaceManager::GetFramebuffer(const RenderPassBase& rp, VkDevice device, Swapchain& swapchain)
{
	for (const auto& surfDesc : rp.GetSurfaceDescriptions())
	{

	}

}

imp::Surface imp::SurfaceManager::GetSurface(const SurfaceDesc& desc, VkDevice device)
{
	// carry on here
	const auto it = m_SurfacePool.find(desc);
}

void imp::SurfaceManager::CombForUnusedSurfaces()
{
	// unordered map is actually okay, can save the .first values that are old and need to be extracted
}

void imp::SurfaceManager::Destroy(VkDevice device)
{
}
