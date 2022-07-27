#pragma once
#include "Surface.h"
#include "backend/graphics/Framebuffer.h"
#include "backend/graphics/GraphicsCaps.h"
#include <vector>
#include <unordered_map>

namespace imp
{
	class Swapchain;
	class RenderPassBase;

	class SurfaceManager
	{
	public:
		SurfaceManager();
		void Initialize(VkDevice device, MemoryProps deviceMemoryProps);

		Framebuffer GetFramebuffer(const RenderPassBase& rp, VkDevice device, Swapchain& swapchain);
		Surface GetSurface(const SurfaceDesc& desc, VkDevice device);

		void CombForUnusedSurfaces();

		void Destroy(VkDevice device);
	private:

		//std::vector<VkImageView> GetAndEnsureRequestedSurfacesesViews(const std::vector<std::pair<uint32_t, SurfaceDesc>>& requestedSurfaces, const RenderPass& rp, uint32_t swapchainIndex);
		Surface CreateSurface(const SurfaceDesc& desc, VkDevice device);
		Framebuffer CreateFramebuffer(const RenderPassBase& rp, const std::vector<Surface>& surfaces, VkDevice device);

		std::unordered_map<SurfaceDesc, Surface> m_SurfacePool;
		MemoryProps m_MemoryProps;
	};
}
