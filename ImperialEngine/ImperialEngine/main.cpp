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

	while (!engine.ShouldClose())
	{
		engine.StartFrame();
		engine.Update();
		engine.Render();
		engine.EndFrame();
	}

	engine.ShutDown();
	return 0;
}