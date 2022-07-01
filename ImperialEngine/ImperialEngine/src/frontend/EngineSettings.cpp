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
		break;
	case EngineSettingsTemplate::kEngineSettingsDevelopment:
		threadingMode = kEngineMultiThreaded;
		break;
	case EngineSettingsTemplate::kEngineSettingsRelease:
		threadingMode = kEngineMultiThreaded;
		break;
	default:
		*this = {};
	}

	// common
}
