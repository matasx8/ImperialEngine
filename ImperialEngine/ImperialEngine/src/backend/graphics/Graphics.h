#pragma once
#include "backend/graphics/CommandBufferManager.h"
#include "backend/graphics/SurfaceManager.h"
#include "backend/graphics/GraphicsCaps.h"
#include "backend/graphics/RenderPass.h"
#include "backend/graphics/Swapchain.h"
#include "backend/graphics/VkDebug.h"
#include "backend/VkWindow.h"
#include "Utils/NonCopyable.h"

namespace imp
{
	class Window;

	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings, Window* window);

		void PrototypeRenderPass();
		void EndFrame();

		void Destroy();

	private:


		void CreateInstance();
		void FindPhysicalDevice();
		void CreateLogicalDevice();
		void CreateVkWindow(Window* window);
		void CreateSwapchain();
		void CreateCommandBufferManager();
		void CreateSurfaceManager();

		bool CheckExtensionsSupported(std::vector<const char*> extensions);

		EngineGraphicsSettings m_Settings;
		GraphicsCaps m_GfxCaps;
		ValidationLayers m_ValidationLayers;

		VkInstance m_VkInstance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_LogicalDevice;

		// These are here for now
		// should save the indices too
		VkQueue m_GfxQueue;
		VkQueue m_PresentationQueue;

		Swapchain m_Swapchain;

		CommandBufferManager m_CbManager;
		SurfaceManager m_SurfaceManager;

		VkWindow m_Window;

		friend class RenderPassBase;
		friend class RenderPass;
		// prototyping..
		RenderPassBase* renderpass;
	};
}