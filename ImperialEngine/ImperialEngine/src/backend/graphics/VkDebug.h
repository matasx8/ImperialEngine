#pragma once
#include "volk.h"
#include "frontend/EngineSettings.h"

namespace imp
{

	class ValidationLayers
	{
	public:
		ValidationLayers();
		void EnableCallback(VkInstance instance, const EngineGraphicsSettings& settings);
		static VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo(const EngineGraphicsSettings& settings);
		void Destroy(VkInstance instance);

	private:
		VkDebugUtilsMessengerEXT m_DebugMessenger;
	};
}