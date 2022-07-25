#include "SurfaceManager.h"

imp::SurfaceManager::SurfaceManager()
{
}

void imp::SurfaceManager::Initialize(VkDevice device)
{
}

VkFramebuffer imp::SurfaceManager::GetFramebuffer(const RenderPassBase& rp, VkDevice device, Swapchain& swapchain)
{
	return VkFramebuffer();
}

void imp::SurfaceManager::CombForUnusedSurfaces()
{
}

void imp::SurfaceManager::Destroy(VkDevice device)
{
}
