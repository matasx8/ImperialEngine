#pragma once
#include <vulkan.h>
#include <vector>
#include <array>
#include "Utils/NonCopyable.h"
#include "frontend/EngineSettings.h"
#include "backend/graphics/GraphicsCaps.h"
#include "backend/graphics/Surface.h"

namespace imp
{
	class Swapchain : NonCopyable
	{
	public:
		Swapchain();
		void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface, const EngineGraphicsSettings& gfxSettings, const PhysicalDeviceSurfaceCaps& surfaceCaps, VkExtent2D extent);

		void Present();

		SurfaceDesc GetSwapchainImageSurfaceDesc();

		void Destroy(VkDevice device);
	private:

		void PopulateNewSwapchainImages(VkDevice device);

		VkSwapchainKHR m_Swapchain;
		VkSurfaceFormatKHR m_Format;
		VkExtent2D m_Extent;
		VkPresentModeKHR m_PresentMode;
		uint32_t m_ImageCount;

		std::array<Surface, kEngineSwapchainExclusiveMax - 1> m_SwapchainImages;
	};
}