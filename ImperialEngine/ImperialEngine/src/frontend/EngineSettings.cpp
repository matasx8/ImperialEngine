#include "EngineSettings.h"
#include <vulkan_core.h>

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
	VK_KHR_SWAPCHAIN_EXTENSION_NAME//,
	//VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,	// TODO: Add some system that removes nsight if not available because when launching renderdoc it crashes my app because it doesnt have these extensions for some reason
	//VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME
	};
	gfxSettings.swapchainImageCount = kEngineSwapchainTripleBuffering;
	gfxSettings.preferredPresentModes = { kEnginePresentMailbox, kEnginePresentFifo };
	gfxSettings.renderMode = static_cast<EngineRenderMode>(kDefaultEngineRenderMode);
}
