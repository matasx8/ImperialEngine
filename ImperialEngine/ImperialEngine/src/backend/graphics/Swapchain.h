#pragma once
#include "volk.h"
#include <vector>
#include <array>
#include "Utils/NonCopyable.h"
#include "frontend/EngineSettings.h"
#include "backend/graphics/GraphicsCaps.h"
#include "backend/graphics/Surface.h"
#include "backend/graphics/Semaphore.h"

namespace imp
{
	template<typename T,
		typename FactoryFunction>
	class PrimitivePool;
	class Swapchain : NonCopyable
	{
	public:
		Swapchain(PrimitivePool<Semaphore, SemaphoreFactory>& semaphorePool);
		void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR windowSurface, const EngineGraphicsSettings& gfxSettings, const PhysicalDeviceSurfaceCaps& surfaceCaps, VkExtent2D extent);

		void Present(VkQueue presentQ, const std::vector<VkSemaphore>& semaphores);
		void AcquireNextImage(VkDevice device, uint64_t currentFrame);

		// TODO: workaround:
		void UpdateSwapchainImage(Surface& surf);

		SurfaceDesc GetSwapchainImageSurfaceDesc() const;
		Surface& GetSwapchainImageSurface(VkDevice device, uint64_t currFrame);
		uint32_t GetSwapchainImageCount() const;
		uint32_t GetFrameClock() const;

		void Destroy(VkDevice device);
	private:

		void PopulateNewSwapchainImages(VkDevice device);

		VkSwapchainKHR m_Swapchain;
		VkSurfaceFormatKHR m_Format;
		VkExtent2D m_Extent;
		VkPresentModeKHR m_PresentMode;
		uint32_t m_ImageCount;
		uint32_t m_SwapchainIndex;
		uint32_t m_FrameClock;
		bool m_NeedsAcquiring;

		std::array<Surface, kEngineSwapchainExclusiveMax - 1> m_SwapchainImages;
		std::array<VkSemaphore, kEngineSwapchainExclusiveMax - 1> m_Semaphores;
		PrimitivePool<Semaphore, SemaphoreFactory>& m_SemaphorePool;
	};
}