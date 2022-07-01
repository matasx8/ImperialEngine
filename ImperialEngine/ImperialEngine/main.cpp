#include<iostream>
#include "frontend/Engine.h"

int main()
{
	EngineSettings settings;
#if _DEBUG
	settings = { EngineSettingsTemplate::kEngineSettingsDebug };
#elif _DEV
	settings = { EngineSettingsTemplate::kEngineSettingsDevelopment };
#elif NDEBUG
	settings = { EngineSettingsTemplate::kEngineSettingsRelease };
#endif
	imp::Engine engine;
	
	// maybe return error code or something like dat, so user can try to reinit with dif settings
	if (!engine.Initialize(settings))
		return 1;

	// insert 'shouldclose' or something like that
	// should leave somekind of input interface so user can decide when to close
	int iterations = 1234;
	while (iterations--)
	{
		engine.Update();
		engine.Render();
	}

	engine.ShutDown();
	return 0;
}