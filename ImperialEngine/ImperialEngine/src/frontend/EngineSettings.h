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

struct EngineGraphicsSettings
{
	std::vector<const char*> requiredExtensions;
	std::vector<const char*> requiredDeviceExtensions;
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

