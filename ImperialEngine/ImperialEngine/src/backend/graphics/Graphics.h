#pragma once
#include "backend/graphics/GraphicsCaps.h"
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

		void Destroy();

	private:

		void CreateInstance();
		void FindPhysicalDevice();
		void CreateLogicalDevice();
		void CreateVkWindow(Window* window);

		bool CheckExtensionsSupported(std::vector<const char*> extensions);

		EngineGraphicsSettings m_Settings;
		GraphicsCaps m_GfxCaps;
		ValidationLayers m_ValidationLayers;

		VkInstance m_VkInstance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_LogicalDevice;

		// These are here for now
		VkQueue m_GfxQueue;
		VkQueue m_PresentationQueue;

		VkWindow m_Window;
	};
}