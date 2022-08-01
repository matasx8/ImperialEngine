#include<iostream>
#include "frontend/Engine.h"
#include <chrono>
#include <thread>

int main()
{
	//std::this_thread::sleep_for(std::chrono::milliseconds(15000));

	EngineSettings settings;
#if _DEBUG && !_DEV // for some reason _DEBUG is not removed
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
		engine.Render();
		engine.EndFrame();
		engine.SyncRenderThread();
		engine.Update();
		engine.SyncGameThread();
	}

	engine.ShutDown();
	return 0;
}