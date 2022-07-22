#pragma once
#include <string>
#include <vector>

enum class EngineSettingsTemplate
{
	kEngineSettingsDebug,
	kEngineSettingsDevelopment, // debug and multithread
	kEngineSettingsRelease
};

enum EngineThreadingMode
{
	kEngineSingleThreaded,
	kEngineMultiThreaded
};

enum EngineSwapchainImageCount
{
	kEngineSwapchainDoubleBuffering = 2,
	kEngineSwapchainTripleBuffering,
	kEngineSwapchainExclusiveMax		// convenience for arrays
};

enum EnginePresentMode
{
	kEnginePresentFifo,
	kEnginePresentMailbox
};

struct EngineGraphicsSettings
{
	std::vector<const char*> requiredExtensions;
	std::vector<const char*> requiredDeviceExtensions;
	std::vector<EnginePresentMode> preferredPresentModes;	// sorted list of preferred present modes, first available is chosen
	EngineSwapchainImageCount swapchainImageCount;
	bool validationLayersEnabled;
};

// engine settings used for initialization
// can configure everything yourself or use a predefined template configured by EngineSettingsTemplate
struct EngineSettings
{
	EngineSettings();
	EngineSettings(EngineSettingsTemplate settingsTemplate);
	
	EngineThreadingMode threadingMode;
	EngineGraphicsSettings gfxSettings;
};

