#pragma once
#include "backend/graphics/GraphicsCaps.h"
#include "backend/graphics/VkDebug.h"
#include "Utils/NonCopyable.h"

namespace imp
{
	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings);

		void Destroy();

	private:

		bool CheckExtensionsSupported(std::vector<const char*> extensions);

		EngineGraphicsSettings m_Settings;
		GraphicsCaps m_GfxCaps;
		ValidationLayers m_ValidationLayers;

		VkInstance m_VkInstance;
	};
}