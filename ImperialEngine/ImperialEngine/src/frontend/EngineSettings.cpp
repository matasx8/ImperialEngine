#include "EngineSettings.h"
#include "volk.h"

EngineSettings::EngineSettings()
	: threadingMode(kEngineSingleThreaded)
{
}

EngineSettings::EngineSettings(EngineSettingsTemplate settingsTemplate)
{
	switch (settingsTemplate)
	{
	case EngineSettingsTemplate::kEngineSettingsDebug:
		threadingMode = kEngineSingleThreaded;
		gfxSettings.validationLayersEnabled = true;
		break;
	case EngineSettingsTemplate::kEngineSettingsDevelopment:
		threadingMode = kEngineMultiThreaded;
		gfxSettings.validationLayersEnabled = true;
		break;
	case EngineSettingsTemplate::kEngineSettingsRelease:
		threadingMode = kEngineMultiThreaded;
		gfxSettings.validationLayersEnabled = false;
		break;
	default:
		*this = {};
	}

	// common
	gfxSettings.requiredDeviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_NV_MESH_SHADER_EXTENSION_NAME
	};
	gfxSettings.swapchainImageCount = kEngineSwapchainDoubleBuffering;
	
#if !BENCHMARK_MODE
	gfxSettings.preferredPresentModes = { /*kEnginePresentMailbox,*/ kEnginePresentFifo };
#else
	// can get more stable results without vsync
	gfxSettings.preferredPresentModes = { kEnginePresentMailbox, kEnginePresentFifo};
#endif
	gfxSettings.renderMode = static_cast<EngineRenderMode>(kDefaultEngineRenderMode);
}

std::string EngineGraphicsSettings::RenderingModeToString(EngineRenderMode mode)
{
	switch (mode)
	{
	case kEngineRenderModeTraditional:
		return "Traditional";
	case kEngineRenderModeGPUDriven:
		return "GPU-Driven";
	case kEngineRenderModeGPUDrivenMeshShading:
		return "GPU-Driven-Mesh";
	case kEngineRenderModeCount:
		return "Render-Mode-Count";
	}
}
