#pragma once
#include <vector>
#include "Surface.h"

class SurfaceManager
{
public:
	SurfaceManager(VulkanRenderer& engine);
	void CreateSwapchainSurfaces(VkSurfaceKHR windowSurface, SwapchainInfo swapchain);

	VkExtent2D GetSwapchainExtent() const;
	VkSwapchainKHR GetSwapchain() const;
	// Query for Framebuffer.
	// Will get surfaces from pool or create new ones.
	// TODO: cache framebuffers also. To save time now create a new FB each query.
	VkFramebuffer GetFramebuffer(const RenderPass& rp, uint32_t swapchainIndex = ~0ull);

	Surface GetSurface(uint32_t surfaceSlot, uint32_t swapchainIndex = 0);

	void CombForUnusedSurfaces();

	void CleanUp();
private:

	std::vector<VkImageView> GetAndEnsureRequestedSurfacesesViews(const std::vector<std::pair<uint32_t, SurfaceDesc>>& requestedSurfaces, const RenderPass& rp, uint32_t swapchainIndex);
	Surface CreateSurface(const SurfaceDesc& desc);
	VkFramebuffer CreateFramebuffer(const std::vector<VkImageView>& imageViews, const RenderPass& rp);


	VulkanRenderer& m_GfxEngine;
	// For now have it passed in. Later figure out how to create it from here obviously
	VkSurfaceKHR m_WindowSurface;
	SwapchainInfo m_Swapchain;

	// I think it's important to have constant time search, so will pick this.
	// Drawback is iteration over each surface each frame to see if surfaces are 
	// not outdated and need to be thrown out.
	std::unordered_map<uint32_t, Surface> m_SurfacePool;
	std::vector<Surface> m_SwapchainSurfaces;
};

