#include "EngineSettings.h"

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
}
