#pragma once
#include "frontend/EngineSettings.h"
#include "backend/graphics/GraphicsCaps.h"
#include "Utils/NonCopyable.h"
#include "vulkan.h"

namespace imp
{
	class Graphics : NonCopyable
	{
	public:
		Graphics();
		void Initialize(const EngineGraphicsSettings& settings);

	private:

		bool CheckExtensionsSupported(std::vector<const char*> extensions);

		EngineGraphicsSettings m_Settings;
		GraphicsCaps m_GfxCaps;

		VkInstance m_VkInstance;
	};
}