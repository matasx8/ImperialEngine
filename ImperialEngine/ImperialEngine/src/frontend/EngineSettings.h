#pragma once

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

// engine settings used for initialization
// can configure everything yourself or use a predefined template configured by EngineSettingsTemplate
struct EngineSettings
{
	EngineSettings();
	EngineSettings(EngineSettingsTemplate settingsTemplate);
	
	EngineThreadingMode threadingMode;
};

