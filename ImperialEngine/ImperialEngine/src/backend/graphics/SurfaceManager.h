#pragma once
#include <vector>
#include "Surface.h"
#include <unordered_map>

namespace imp
{
	class Swapchain;
	class RenderPassBase;

	class SurfaceManager
	{
	public:
		SurfaceManager();
		void Initialize(VkDevice device);

		VkFramebuffer GetFramebuffer(const RenderPassBase& rp, VkDevice device, Swapchain& swapchain);

		void CombForUnusedSurfaces();

		void Destroy(VkDevice device);
	private:

		std::vector<VkImageView> GetAndEnsureRequestedSurfacesesViews(const std::vector<std::pair<uint32_t, SurfaceDesc>>& requestedSurfaces, const RenderPass& rp, uint32_t swapchainIndex);
		Surface CreateSurface(const SurfaceDesc& desc);
		VkFramebuffer CreateFramebuffer(const std::vector<VkImageView>& imageViews, const RenderPass& rp);

		// I think it's important to have constant time search, so will pick this.
		// Drawback is iteration over each surface each frame to see if surfaces are 
		// not outdated and need to be thrown out.
		std::unordered_map<uint32_t, Surface> m_SurfacePool;
	};
}
